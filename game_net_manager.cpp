#include "stdafx.h"
#include "main.h"
#include "common_func.h"
#include "game_net_manager.h"
#include "robot_utils.h"
#include "base_room.h"
#include "user.h"
#include "room_manager.h"
#include "user_manager.h"


int GameNetManager::Init(const std::string& game_ip, const int& game_port) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (kCommSucc != CheckNotInnerThread()) {
        ASSERT_FALSE_RETURN
    }
    CHECK_GAMEIP(game_ip);
    CHECK_GAMEPORT(game_port);

    // 同步过程
    game_ip_ = game_ip;
    game_port_ = game_port;

    if (kCommSucc != InitDataWithLock()) {
        ASSERT_FALSE_RETURN;
    }

    // @zhuhangmin 20190225 不可颠倒同步过程和接收消息顺序。不然前置通知消息会被SendGetGameInfo覆盖
    notify_thread_.Initial(std::thread([this] {this->ThreadGameInfoNotify(); }));

    heart_thread_.Initial(std::thread([this] {this->ThreadSendGamePulse(); }));

    return kCommSucc;
}

int GameNetManager::Term() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (kCommSucc != CheckNotInnerThread()) {
        ASSERT_FALSE_RETURN
    }
    notify_thread_.Release();
    heart_thread_.Release();
    LOG_FUNC("[EXIT ROUTINE]");
    return kCommSucc;
}



int GameNetManager::SendGameRequest(const RequestID requestid, const google::protobuf::Message &val, REQUEST& response, const bool bNeedEcho /*= true*/) {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_REQUESTID(requestid);
    return RobotUtils::SendRequestWithLock(connection_, requestid, val, response, bNeedEcho);
}

int GameNetManager::ThreadGameInfoNotify() {
    LOG_INFO("[START ROUTINE] GameNotify thread [%d] started", GetCurrentThreadId());
    MSG msg = {};
    while (GetMessage(&msg, 0, 0, 0)) {
        if (UM_DATA_RECEIVED == msg.message) {

            LPCONTEXT_HEAD	pContext = (LPCONTEXT_HEAD) (msg.wParam);
            LPREQUEST		pRequest = (LPREQUEST) (msg.lParam);
            RequestID		requestid = pRequest->head.nRequest;

            OnGameInfoNotify(requestid, *pRequest);

            SAFE_DELETE(pContext);
            UwlClearRequest(pRequest);
            SAFE_DELETE(pRequest);
        } else {
            DispatchMessage(&msg);
        }
    }
    LOG_INFO("[EXIT ROUTINE] GameNotify thread [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}

int GameNetManager::OnGameInfoNotify(const RequestID requestid, const REQUEST &request) {
    switch (requestid) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
            OnDisconnGameInfo();
            break;
        case GN_RS_PLAER_ENTERGAME:
            OnPlayerEnterGame(request);
            break;
        case GN_RS_LOOKER_ENTERGAME:
            OnLookerEnterGame(request);
            break;
        case GN_RS_LOOER2PLAYER:
            OnLooker2Player(request);
            break;
        case GN_RS_PLAYER2LOOKER:
            OnPlayer2Looker(request);
            break;
        case GN_RS_GAME_START:
            OnStartGame(request);
            break;
        case GN_RS_USER_REFRESH_RESULT:
            OnUserFreshResult(request);
            break;
        case GN_RS_REFRESH_RESULT:
            OnFreshResult(request);
            break;
        case GN_RS_USER_LEAVEGAME:
            OnLeaveGame(request);
            break;
        case GN_RS_SWITCH_TABLE:
            OnSwitchTable(request);
            break;
        default:
            break;

    }

    return kCommSucc;
}

int GameNetManager::OnDisconnGameInfo() {
    std::lock_guard<std::mutex> lock(mutex_);
    ResetInitDataWithLock();
    return kCommSucc;
}

int GameNetManager::ThreadSendGamePulse() {
    LOG_INFO("[START ROUTINE] Game KeepAlive thread [%d] started", GetCurrentThreadId());
    while (true) {
        DWORD dwRet = WaitForSingleObject(g_hExitServer, PulseInterval);
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }

        if (WAIT_TIMEOUT == dwRet) {
            SendPulse();
        }
    }
    LOG_INFO("[EXIT ROUTINE] Game KeepAlive thread [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}

// 具体业务
int GameNetManager::SendValidateReqWithLock() {
    game::base::RobotSvrValidateReq val;
    val.set_client_id(g_nClientID);
    REQUEST response = {};
    auto result = SendGameRequest(GR_VALID_ROBOTSVR, val, response);

    if (kCommSucc != result) {
        ASSERT_FALSE;
        if (RobotErrorCode::kOperationFailed == result) {
            ResetInitDataWithLock();
        }
        return result;
    }

    game::base::RobotSvrValidateResp resp;
    int ret = ParseFromRequest(response, resp);
    if (kCommSucc != ret) {
        LOG_ERROR("ParseFromRequest faild.");
        return kCommFaild;
    }

    if (kCommSucc != resp.code()) {
        LOG_ERROR("SendValidateReq faild. check return[%d]. req = %s", resp.code(), GetStringFromPb(val).c_str());
        return kCommFaild;
    }

    return kCommSucc;
}

int GameNetManager::SendGetGameInfoWithLock() {
    game::base::GetGameUsersReq val;
    val.set_clientid(g_nClientID);
    val.set_roomid(0);
    REQUEST response = {};
    auto result = SendGameRequest(GR_GET_GAMEUSERS, val, response);
    if (kCommSucc != result) {
        ASSERT_FALSE;
        if (RobotErrorCode::kOperationFailed == result) {
            ResetInitDataWithLock();
        }
        return result;
    }

    game::base::GetGameUsersResp resp;
    int ret = ParseFromRequest(response, resp);
    if (kCommSucc != ret) {
        LOG_ERROR("ParseFromRequest faild.");
        return kCommFaild;
    }

    if (kCommSucc != resp.code()) {
        LOG_ERROR("SendGetGameInfo faild. check return[%d]. req = %s", resp.code(), GetStringFromPb(val).c_str());
        return kCommFaild;
    }

    RoomMgr.Reset();
    for (int room_index = 0; room_index < resp.rooms_size(); room_index++) {
        game::base::Room room_pb = resp.rooms(room_index);
        AddRoomPB(room_pb);
    }

    UserMgr.Reset();
    for (int user_index = 0; user_index < resp.users_size(); user_index++) {
        game::base::User user_pb = resp.users(user_index);
        AddUserPB(user_pb);
    }

    return kCommSucc;
}

int GameNetManager::InitDataWithLock() {
    if (kCommSucc != ConnectGameSvrWithLock(game_ip_, game_port_)) {
        LOG_ERROR("ConnectGame failed");
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != SendValidateReqWithLock()) {
        LOG_ERROR("SendValidateReq failed");
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != SendGetGameInfoWithLock()) {
        LOG_ERROR("SendGetGameInfo failed");
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int GameNetManager::ResetDataWithLock() {
    if (connection_) {
        //@zhuhanmin 20190228 socket建立后发消息时如果连接已断失效，销毁
        connection_->DestroyEx();
    }
    timeout_count_ = 0;
    UserMgr.Reset();
    RoomMgr.Reset();
    return kCommSucc;
}

int GameNetManager::SendPulse() {
    std::lock_guard<std::mutex> lock(mutex_);

    game::base::PulseReq val;
    val.set_id(g_nClientID);
    REQUEST response = {};

    //@zhuhangmin 大厅模板需要支持 清僵尸机制
    auto result = RobotUtils::SendRequestWithLock(connection_, GR_GAME_PLUSE, val, response, false);

    if (kCommSucc != result) {
        if (result == RobotErrorCode::kConnectionTimeOut) {
            timeout_count_++;
            if (timeout_count_ == MaxPluseTimeOutCount) {
                ResetInitDataWithLock();
            }
            return RobotErrorCode::kConnectionTimeOut;
        }
    }

    return result;
}

int GameNetManager::OnPlayerEnterGame(const REQUEST &request) {
    game::base::RS_UserEnterGameNotify ntf;
    int parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        LOG_WARN("ParseFromRequest failed.");
        ASSERT_FALSE_RETURN;
    }

    game::base::RoomData room_data = ntf.room_data();
    auto roomid = room_data.roomid();

    //V614 The 'user' smart pointer is utilized immediately after being declared or reset.
    //It is suspicious that no value was assigned to it.
    auto user = std::make_shared<User>();
    user->set_user_id(ntf.userid());
    user->set_table_no(ntf.tableno());
    user->set_chair_no(ntf.chairno());
    user->set_user_type(ntf.user_type());
    user->set_room_id(roomid);

    RoomPtr base_room;
    if (kCommSucc != RoomMgr.GetRoom(roomid, base_room)) {
        LOG_WARN("GetRoom failed room");
        ASSERT_FALSE_RETURN;
    }

    int ret = base_room->PlayerEnterGame(user); //Add User Manager
    if (ret != kCommSucc) {
        LOG_WARN("PlayerEnterGame failed.");
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != UserMgr.AddUser(user->get_user_id(), user)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int GameNetManager::OnLookerEnterGame(const REQUEST &request) {
    game::base::RS_UserEnterGameNotify ntf;
    int parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        LOG_WARN("ParseFromRequest failed.");
        ASSERT_FALSE_RETURN;
    }

    game::base::RoomData room_data = ntf.room_data();
    auto roomid = room_data.roomid();

    //V614 The 'user' smart pointer is utilized immediately after being declared or reset.
    //It is suspicious that no value was assigned to it.
    auto user = std::make_shared<User>();
    user->set_user_id(ntf.userid());
    user->set_room_id(roomid);
    user->set_table_no(ntf.tableno());
    user->set_chair_no(ntf.chairno());
    user->set_user_type(ntf.user_type());

    RoomPtr base_room;
    if (kCommSucc != RoomMgr.GetRoom(roomid, base_room)) {
        LOG_WARN("GetRoom failed room");
        ASSERT_FALSE_RETURN;
    }

    int ret = base_room->LookerEnterGame(user); //Add User To User Manager
    if (ret != kCommSucc) {
        LOG_WARN("PlayerEnterGame failed.");
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != UserMgr.AddUser(user->get_user_id(), user)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int GameNetManager::OnLooker2Player(const REQUEST &request) {
    game::base::RS_SwitchLookerPlayerNotify ntf;
    int parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        LOG_WARN("ParseFromRequest failed.");
        ASSERT_FALSE_RETURN;
    }

    auto userid = ntf.userid();
    auto roomid = ntf.roomid();
    auto tableno = ntf.tableno();
    auto chairno = ntf.chairno();

    RoomPtr base_room;
    if (kCommSucc != RoomMgr.GetRoom(roomid, base_room)) {
        LOG_WARN("GetRoom failed room");
        ASSERT_FALSE_RETURN;
    }

    UserPtr user;
    if (kCommSucc != UserMgr.GetUser(userid, user)) {
        ASSERT_FALSE_RETURN;
    }
    user->set_room_id(roomid);
    user->set_table_no(tableno);
    user->set_chair_no(chairno);

    int ret = base_room->Looker2Player(user);
    if (ret != kCommSucc) {
        LOG_WARN("Looker2Player failed");
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int GameNetManager::OnPlayer2Looker(const REQUEST &request) {
    game::base::RS_SwitchLookerPlayerNotify ntf;
    int parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        LOG_WARN("ParseFromRequest failed.");
        ASSERT_FALSE_RETURN;
    }

    auto userid = ntf.userid();
    auto roomid = ntf.roomid();
    auto tableno = ntf.tableno();
    auto chairno = ntf.chairno();

    RoomPtr base_room;
    if (kCommSucc != RoomMgr.GetRoom(roomid, base_room)) {
        LOG_WARN("GetRoom failed room");
        ASSERT_FALSE_RETURN;
    }

    UserPtr user;
    if (kCommSucc != UserMgr.GetUser(userid, user)) {
        ASSERT_FALSE_RETURN;
    }
    user->set_room_id(roomid);
    user->set_table_no(tableno);
    user->set_chair_no(chairno);

    int ret = base_room->Player2Looker(user);
    if (ret != kCommSucc) {
        LOG_WARN("Player2Looker failed");
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int GameNetManager::OnStartGame(const REQUEST &request) {
    game::base::RS_StartGameNotify ntf;
    int parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        LOG_WARN("ParseFromRequest failed.");
        ASSERT_FALSE_RETURN;
    }

    auto roomid = ntf.roomid();
    auto tableno = ntf.tableno();

    //@zhuhangmin TODO refactor into RoomMgr add param check chari
    std::unordered_map<ChairNO, game::base::ChairInfo> chairs_pb_map;
    for (auto index = 0; index < ntf.chairs_size(); index++) {
        auto chair_pb = ntf.chairs(index);
        chairs_pb_map[chair_pb.chairno()] = chair_pb;
    }

    RoomPtr base_room;
    if (kCommSucc != RoomMgr.GetRoom(roomid, base_room)) {
        LOG_WARN("GetRoom failed room");
        ASSERT_FALSE_RETURN;
    }

    TablePtr table;
    if (kCommSucc != base_room->GetTable(tableno, table)) {
        LOG_WARN("GetTable faild tableno=%d", tableno);
        ASSERT_FALSE_RETURN;
    }

    auto chairs = table->GetChairs();
    for (auto& kv : chairs_pb_map) {
        auto chair_pb = kv.second;
        ChairNO chairno = chair_pb.chairno();
        chairs[chairno].set_userid(chair_pb.userid());
        chairs[chairno].set_chair_status((ChairStatus) chair_pb.chair_status());
    }
    return kCommSucc;
}

int GameNetManager::OnUserFreshResult(const REQUEST &request) {
    game::base::RS_UserRefreshResultNotify ntf;
    int parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        LOG_WARN("ParseFromRequest failed.");
        ASSERT_FALSE_RETURN;
    }

    auto userid = ntf.userid();
    auto roomid = ntf.roomid();
    auto tableno = ntf.tableno();

    RoomPtr base_room;
    if (kCommSucc != RoomMgr.GetRoom(roomid, base_room)) {
        LOG_WARN("GetRoom failed room");
        ASSERT_FALSE_RETURN;
    }
    TablePtr table;
    if (kCommSucc != base_room->GetTable(tableno, table)) {
        LOG_WARN("GetTable faild. tableno=%d", tableno);
        ASSERT_FALSE_RETURN;
    }

    table->RefreshGameResult(userid);
    return kCommSucc;
}

int GameNetManager::OnFreshResult(const REQUEST &request) {
    game::base::RS_RefreshResultNotify ntf;
    int parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        LOG_WARN("ParseFromRequest failed.");
        ASSERT_FALSE_RETURN;
    }

    auto roomid = ntf.roomid();
    auto tableno = ntf.tableno();

    RoomPtr base_room;
    if (kCommSucc != RoomMgr.GetRoom(roomid, base_room)) {
        LOG_WARN("GetRoom failed room");
        ASSERT_FALSE_RETURN;
    }

    TablePtr table;
    if (kCommSucc != base_room->GetTable(tableno, table)) {
        LOG_WARN("GetTable faild.  tableno=%d", tableno);
        ASSERT_FALSE_RETURN;
    }
    table->RefreshGameResult();
    return kCommSucc;
}

int GameNetManager::OnLeaveGame(const REQUEST &request) {
    game::base::RS_UserLeaveGameNotify ntf;
    int parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        LOG_WARN("ParseFromRequest failed.");
        ASSERT_FALSE_RETURN;
    }

    auto userid = ntf.userid();
    auto roomid = ntf.roomid();
    auto tableno = ntf.tableno();

    RoomPtr base_room;
    if (kCommSucc != RoomMgr.GetRoom(roomid, base_room)) {
        LOG_WARN("GetRoom failed room");
        ASSERT_FALSE_RETURN;
    }

    base_room->UserLeaveGame(userid, tableno);

    if (kCommSucc != UserMgr.DelUser(userid)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int GameNetManager::OnSwitchTable(const REQUEST &request) {
    game::base::RS_SwitchTableNotify ntf;
    int parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        LOG_WARN("ParseFromRequest failed.");
        ASSERT_FALSE_RETURN;
    }

    auto userid = ntf.userid();
    auto roomid = ntf.roomid();
    auto old_tableno = ntf.old_tableno();
    auto new_tableno = ntf.new_tableno();
    auto new_chairno = ntf.new_chairno();

    UserPtr user;
    if (kCommSucc != UserMgr.GetUser(userid, user)) {
        ASSERT_FALSE_RETURN;
    }
    user->set_room_id(roomid);
    user->set_table_no(new_tableno);
    user->set_chair_no(new_chairno);

    RoomPtr base_room;
    if (kCommSucc != RoomMgr.GetRoom(roomid, base_room)) {
        LOG_WARN("GetRoom failed room");
        ASSERT_FALSE_RETURN;
    }

    base_room->UnbindUser(userid, old_tableno);
    base_room->BindPlayer(user);
    return kCommSucc;

}
//
//int RobotGameInfoManager::GetUserStatus(UserID userid, UserStatus& user_status) {
//    std::lock_guard<std::mutex> lock(game_info_connection_mutex_);
//
//    if (user_map_.find(userid) == user_map_.end()) return kCommFaild;
//    auto user = user_map_[userid];
//    auto chairno = user.chairno();
//
//    // 玩家信息中token为0则说明玩家离线； kUserOffline = 0x10000000		// 断线
//
//    // 玩家信息中椅子号为0则说明在旁观；
//    if (chairno == 0) {
//        user_status = kUserLooking;
//        return kCommSucc;
//    }
//
//    // 有椅子号则查看桌子状态，桌子waiting -> 玩家waiting
//    game::base::Table table;
//    if (kCommSucc != FindTable(userid, table)) return kCommFaild;
//    auto table_status = table.table_status();
//    if (table_status == kTableWaiting) {
//        user_status = kUserWaiting;
//        return kCommSucc;
//    }
//
//    // 桌子playing && 椅子playing -> 玩家playing
//    game::base::ChairInfo chair;
//    if (kCommSucc != FindChair(userid, chair)) return kCommFaild;
//    auto chair_status = chair.chair_status();
//    if (table_status != kTablePlaying) return kCommFaild;
//
//    if (chair_status == kChairPlaying) {
//        user_status = kUserPlaying;
//        return kCommSucc;
//    }
//
//    // 桌子playing && 椅子waiting -> 等待下局游戏开始（原空闲玩家）
//    if (chair_status == kChairWaiting) {
//        user_status = kUserWaiting;
//        return kCommSucc;
//    }
//
//    return kCommFaild;
//}
//
int GameNetManager::AddRoomPB(const game::base::Room room_pb) {
    game::base::RoomData room_data_pb = room_pb.room_data();
    RoomID roomid = room_data_pb.roomid();

    auto base_room = std::make_shared<BaseRoom>();
    base_room->set_room_id(room_data_pb.roomid());
    base_room->set_options(room_data_pb.options());
    base_room->set_configs(room_data_pb.configs());
    base_room->set_manages(room_data_pb.manages());
    base_room->set_max_table_cout(room_data_pb.max_table_cout());
    base_room->set_chaircount_per_table(room_data_pb.chaircount_per_table());
    base_room->set_min_deposit(room_data_pb.min_deposit());
    base_room->set_max_deposit(room_data_pb.max_deposit());

    // TABLE
    for (int table_index = 0; table_index < room_pb.tables_size(); table_index++) {
        game::base::Table table_pb = room_pb.tables(table_index);
        TableNO tableno = table_pb.tableno();
        auto table = std::make_shared<Table>();
        AddTablePB(table_pb, table);
        base_room->AddTable(tableno, table);
    }

    // ROOM
    if (kCommSucc != RoomMgr.AddRoom(roomid, base_room)) return kCommFaild;
    return kCommSucc;
}

int GameNetManager::AddTablePB(const game::base::Table table_pb, TablePtr table) {
    table->set_table_no(table_pb.tableno()); // tableno start from 1
    table->set_room_id(table_pb.roomid());
    table->set_chair_count(table_pb.chair_count());
    table->set_banker_chair(table_pb.banker_chair());
    table->set_min_deposit(table_pb.min_deposit());
    table->set_max_deposit(table_pb.max_deposit());
    table->set_base_deposit(table_pb.base_deposit());
    table->set_table_status(table_pb.table_status());

    // CHAIR
    for (int chair_index = 0; chair_index < table_pb.chairs_size(); chair_index++) {
        game::base::ChairInfo chair_pb = table_pb.chairs(chair_index);
        ChairNO chairno = chair_pb.chairno(); // chairno start from 1
        ChairInfo chair_info;
        chair_info.set_userid(chair_pb.userid());
        chair_info.set_chair_status((ChairStatus) chair_pb.chair_status());
        table->AddChair(chairno, chair_info);
    }

    // TABLE USER INFO
    for (int table_user_index = 0; table_user_index < table_pb.table_users_size(); table_user_index++) {
        game::base::TableUserInfo table_user_pb = table_pb.table_users(table_user_index);
        UserID userid = table_user_pb.userid();
        TableUserInfo table_user_info;
        table_user_info.set_userid(table_user_pb.userid());
        table_user_info.set_bind_timestamp(table_user_pb.bind_timestamp());
        table->AddTableUserInfo(userid, table_user_info);
    }

    return kCommSucc;
}

int GameNetManager::AddUserPB(const game::base::User user_pb) {
    auto user = std::make_shared<User>();
    user->set_user_id(user_pb.userid());
    user->set_room_id(user_pb.roomid());
    user->set_table_no(user_pb.tableno());
    user->set_chair_no(user_pb.chairno());
    user->set_user_type(user_pb.user_type());
    user->set_deposit(user_pb.deposit());
    user->set_total_bout(user_pb.total_bout());
    user->set_offline_count(user_pb.offline_count());
    user->set_enter_timestamp(user_pb.enter_timestamp());
    if (kCommSucc != UserMgr.AddUser(user->get_user_id(), user)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int GameNetManager::ResetInitDataWithLock() {
    // 重置
    if (kCommSucc != ResetDataWithLock()) {
        ASSERT_FALSE_RETURN;
    }

    // 重新初始化
    if (kCommSucc != InitDataWithLock()) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int GameNetManager::ConnectGameSvrWithLock(const std::string& game_ip, const int game_port) {
    CHECK_GAMEIP(game_ip);
    CHECK_GAMEPORT(game_port);
    connection_->InitKey(KEY_GAMESVR_2_0, ENCRYPT_AES, 0);
    if (!connection_->Create(game_ip.c_str(), game_port, 5, 0, notify_thread_.GetThreadID(), 0, GetHelloData(), GetHelloLength())) {
        LOG_ERROR("[ROUTE] ConnectGame Faild! IP:%s Port:%d", game_ip.c_str(), game_port);
        ASSERT_FALSE_RETURN;
    }

    LOG_INFO("ConnectGame OK! IP:%s Port:%d", game_ip.c_str(), game_port);
    return kCommSucc;
}

int GameNetManager::SnapShotObjectStatus() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("OBJECT ADDRESS = %x", this);
    LOG_INFO("game_info_notify_thread_ [%d]", notify_thread_.GetThreadID());
    LOG_INFO("heart_timer_thread_ [%d]", heart_thread_.GetThreadID());
    LOG_INFO("token [%d]", connection_->GetTokenID());

    return kCommSucc;
}

int GameNetManager::CheckNotInnerThread() {
    CHECK_NOT_THREAD(notify_thread_);
    CHECK_NOT_THREAD(heart_thread_);
    return kCommSucc;
}

