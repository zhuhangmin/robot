#include "stdafx.h"
#include "main.h"
#include "common_func.h"
#include "game_net_manager.h"
#include "robot_utils.h"
#include "base_room.h"
#include "user.h"
#include "robot_statistic.h"
#include "PBReq.h"
#include "deposit_data_manager.h"
#include "table.h"


int GameNetManager::Init(const std::string& game_ip, const int& game_port) {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_GAMEIP(game_ip);
    CHECK_GAMEPORT(game_port);

    game_ip_ = game_ip;
    game_port_ = game_port;
    notify_thread_.Initial(std::thread([this] {this->ThreadNotify(); }));
    timer_thread_.Initial(std::thread([this] {this->ThreadTimer(); }));

    if (kCommSucc != InitDataWithLock()) {
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int GameNetManager::Term() {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    notify_thread_.Release();
    timer_thread_.Release();
    game_ip_.clear();
    timeout_count_ = 0;
    sync_time_stamp_ = 0;
    timestamp_ = 0;
    game_port_ = InvalidPort;
    need_reconnect_ = false;
    game_data_inited_ = false;
    if (connection_) {
        LOG_WARN("token [%d] [%s] DestroyEx", connection_->GetTokenID(), __FUNCTION__);
        connection_->DestroyEx();
    }

    LOG_INFO_FUNC("[EXIT]");
    return kCommSucc;
}



int GameNetManager::SendRequestWithLock(const RequestID& requestid, const google::protobuf::Message &val, REQUEST& response, const bool& need_echo /*= true*/) const {
    CHECK_REQUESTID(requestid);
    return RobotUtils::SendRequestWithLock(connection_, requestid, val, response, need_echo);
}

int GameNetManager::ThreadNotify() {
    LOG_INFO("\t[START] GameNotify thread [%d] started", GetCurrentThreadId());
    MSG msg = {};
    while (GetMessage(&msg, 0, 0, 0)) {
        if (UM_DATA_RECEIVED == msg.message) {
            auto pContext = reinterpret_cast<LPCONTEXT_HEAD>(msg.wParam);
            auto pRequest = reinterpret_cast<LPREQUEST>(msg.lParam);
            const auto requestid = pRequest->head.nRequest;

            OnNotify(requestid, *pRequest);

            SAFE_DELETE(pContext);
            UwlClearRequest(pRequest);
            SAFE_DELETE(pRequest);
        } else {
            DispatchMessage(&msg);
        }
    }
    LOG_INFO("[EXIT] GameNotify thread [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}

int GameNetManager::OnNotify(const RequestID& requestid, const REQUEST &request) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        CHECK_REQUESTID(requestid);
        if (PB_NOTIFY_TO_CLIENT != requestid) {
            if (connection_) {
                if (UR_SOCKET_ERROR == requestid || UR_SOCKET_CLOSE == requestid) {
                    LOG_WARN("token [%d] [RECV] [%d] [%s]", connection_->GetTokenID(), requestid, REQ_STR(requestid));
                } else {
                    LOG_INFO("token [%d] [RECV] [%d] [%s]", connection_->GetTokenID(), requestid, REQ_STR(requestid));
                }
            }
        }

        if (!game_data_inited_) {
            LOG_WARN("game room and user data has inited yet kExceptionGameDataNotInited");
            LOG_WARN("token [%d] [RECV] [%d] [%s]", connection_->GetTokenID(), requestid, REQ_STR(requestid));
            return kExceptionGameDataNotInited;
        }
    }

    int result = kCommSucc;
    switch (requestid) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
            result = OnDisconnect();
            break;
        case GN_RS_PLAER_ENTERGAME:
            result = OnPlayerEnterGame(request);
            break;
        case GN_RS_LOOKER_ENTERGAME:
            result = OnLookerEnterGame(request);
            break;
        case GN_RS_LOOER2PLAYER:
            result = OnLooker2Player(request);
            break;
        case GN_RS_PLAYER2LOOKER:
            result = OnPlayer2Looker(request);
            break;
        case GN_RS_GAME_START: // 开桌 对称 GN_RS_REFRESH_RESULT 
            result = OnStartGame(request);
            break;
        case GN_RS_USER_REFRESH_RESULT:
            result = OnUserFreshResult(request);
            break;
        case GN_RS_REFRESH_RESULT:
            result = OnFreshResult(request);
            break;
        case GN_RS_USER_LEAVEGAME:
            result = OnLeaveGame(request);
            break;
        case GN_RS_SWITCH_TABLE:
            result = OnSwitchTable(request);
            break;
        case GN_RS_NEW_ROOM:
            result = OnNewRoom(request);
            break;
        default:
            LOG_WARN("requestid  = [%d]", requestid);
            break;
    }

    EVENT_TRACK(EventType::kRecv, requestid);

    if (kCommSucc != result) {
        LOG_ERROR("requestid  = [%d][%s], result  = [%d]", requestid, REQ_STR(requestid), result, ERR_STR(result));
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int GameNetManager::OnDisconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_THREAD(notify_thread_);
    ResetDataWithLock();
    LOG_WARN("OnDisconnect Game Info");
    return kCommSucc;
}

int GameNetManager::ThreadTimer() {
    LOG_INFO("\t[START] Game KeepAlive thread [%d] started", GetCurrentThreadId());
    while (true) {
        const auto dwRet = WaitForSingleObject(g_hExitServer, TimerInterval);
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }

        if (WAIT_TIMEOUT == dwRet) {
            // 保活
            if (kCommSucc != KeepConnection()) continue;

            // 心跳
            SendPulse();
        }
    }
    LOG_INFO("[EXIT] Game KeepAlive thread [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}

// 具体业务
int GameNetManager::SendValidateReqWithLock() {
    game::base::RobotSvrValidateReq val;
    val.set_client_id(g_nClientID);
    REQUEST response = {};
    const auto result = SendRequestWithLock(GR_RS_VALID_ROBOTSVR, val, response);

    if (kCommSucc != result) {
        if (kOperationFailed == result) {
            ResetDataWithLock();
        }
        ASSERT_FALSE;
        return result;
    }

    game::base::RobotSvrValidateResp resp;
    const auto ret = ParseFromRequest(response, resp);
    if (kCommSucc != ret) {
        LOG_ERROR("ParseFromRequest faild.");
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != resp.code()) {
        LOG_ERROR("SendValidateReq faild. check return[%d]. req =  [%s]", resp.code(), GetStringFromPb(val).c_str());
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int GameNetManager::SendGetGameInfoWithLock() {
    game::base::GetGameUsersReq val;
    val.set_clientid(g_nClientID);
    val.set_roomid(0);
    REQUEST response = {};
    const auto result = SendRequestWithLock(GR_RS_GET_GAMEUSERS, val, response);
    if (kCommSucc != result) {
        ASSERT_FALSE;
        if (kOperationFailed == result) {
            ResetDataWithLock();
        }
        return result;
    }

    game::base::GetGameUsersResp resp;
    const auto ret = ParseFromRequest(response, resp);
    if (kCommSucc != ret) {
        LOG_ERROR("ParseFromRequest faild.");
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != resp.code()) {
        LOG_ERROR("SendGetGameInfo faild. check return[%d]. req =  [%s]", resp.code(), GetStringFromPb(val).c_str());
        ASSERT_FALSE_RETURN;
    }

    if (0 == resp.rooms_size()) {
        LOG_WARN("No Room From Game Server!")
    }
    room_mgr_.ResetDataAndReInit(resp);

    if (0 == resp.users_size()) {
        LOG_WARN("No Users From Game Server!")
    }
    user_mgr_.ResetDataAndReInit(resp);

    return kCommSucc;
}

int GameNetManager::InitDataWithLock() {
    if (kCommSucc != ConnectWithLock(game_ip_, game_port_)) {
        return kCommFaild;
    }

    if (kCommSucc != SendValidateReqWithLock()) {
        LOG_ERROR("SendValidateReq failed");
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != SendGetGameInfoWithLock()) {
        LOG_ERROR("SendGetGameInfo failed");
        ASSERT_FALSE_RETURN;
    }

    need_reconnect_ = false;
    game_data_inited_ = true;
    return kCommSucc;
}

int GameNetManager::ResetDataWithLock() {
    if (connection_) {
        LOG_WARN("token [%d] [%s] DestroyEx", connection_->GetTokenID(), __FUNCTION__);
        connection_->DestroyEx();
    }
    timeout_count_ = 0;
    need_reconnect_ = true;
    user_mgr_.Reset();
    room_mgr_.Reset();
    game_data_inited_ = false;
    return kCommFaild;
}

int GameNetManager::SendPulse() {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto now = std::time(nullptr);
    if (now - timestamp_ > GamePulseInterval) {
        timestamp_ = now;
        game::base::PulseReq val;
        val.set_id(g_nClientID);
        REQUEST response = {};
        // 机器人总连接心跳需要回包
        const auto result = RobotUtils::SendRequestWithLock(connection_, GR_RS_PULSE, val, response);
        if (kCommSucc != result) {
            if (result != kConnectionTimeOut) {
                ResetDataWithLock();
            } else {
                timeout_count_++;
                if (timeout_count_ == MaxPluseTimeOutCount) {
                    ResetDataWithLock();
                }
                return kConnectionTimeOut;
            }
        }

        return result;
    }
    return kCommSucc;
}

int GameNetManager::KeepConnection() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (need_reconnect_) {
        return InitDataWithLock();
    }
    return kCommSucc;
}

int GameNetManager::OnPlayerEnterGame(const REQUEST &request) {
    game::base::RS_UserEnterGameNotify ntf;
    const auto parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        LOG_WARN("ParseFromRequest failed.");
        ASSERT_FALSE_RETURN;
    }

    // 更新用户
    const auto& user_data = ntf.user();
    const auto userid = user_data.userid();
    const auto roomid = user_data.roomid();
    const auto tableno = user_data.tableno();
    const auto chairno = user_data.chairno();
    const auto user_type = user_data.user_type();
    const auto deposit = user_data.deposit();
    const auto total_bout = user_data.total_bout();
    const auto win_bout = user_data.win_bout();
    const auto loss_bout = user_data.loss_bout();
    const auto offline_count = user_data.offline_count();
    const auto& head_url = user_data.head_url();
    const auto& hardid = user_data.hardid();
    const auto& nick_name = user_data.nick_name();

    LOG_INFO("userid [%d] roomid [%d] tableno [%d] [%s]", userid, roomid, tableno, REQ_STR(request.head.nRequest));


    auto user = std::make_shared<User>();
    user->set_user_id(userid);
    user->set_room_id(roomid);
    user->set_table_no(tableno);
    user->set_chair_no(chairno);
    user->set_user_type(user_type);
    user->set_deposit(deposit);
    user->set_total_bout(total_bout);
    user->set_win_bout(win_bout);
    user->set_loss_bout(loss_bout);
    user->set_offline_count(offline_count);
    user->set_head_url(head_url);
    user->set_hard_id(hardid);
    user->set_nick_name(nick_name);

    if (kCommSucc != user_mgr_.AddUser(userid, user)) {
        ASSERT_FALSE_RETURN;
    }

    //更新房间
    const auto& room_data = ntf.room_data();
    RoomPtr room;
    if (kCommSucc != room_mgr_.GetRoom(room_data.roomid(), room)) {
        LOG_WARN("GetRoom failed room");
        ASSERT_FALSE_RETURN;
    }

    const auto options = room_data.options();
    const auto configs = room_data.configs();
    const auto manages = room_data.manages();
    const auto max_table_cout = room_data.max_table_cout();
    const auto chaircount_per_table = room_data.chaircount_per_table();
    const auto min_deposit = room_data.min_deposit();
    const auto max_deposit = room_data.max_deposit();
    const auto min_player_count = room_data.min_player_count();

    room->set_options(options);
    room->set_configs(configs);
    room->set_manages(manages);
    room->set_max_table_cout(max_table_cout);
    room->set_chaircount_per_table(chaircount_per_table);
    room->set_min_deposit(min_deposit);
    room->set_max_deposit(max_deposit);
    room->set_min_playercount_per_table(min_player_count);


    TablePtr table;
    if (kCommSucc != room->GetTable(tableno, table)) {
        LOG_WARN("GetTable faild. userid  [%d], tableno [%d]", userid, tableno);
        ASSERT_FALSE_RETURN;
    }
    table->set_max_deposit(ntf.max_deposit());
    table->set_min_deposit(ntf.min_deposit());
    table->set_base_deposit(ntf.base_deposit());

    // 上桌
    const auto ret = room->PlayerEnterGame(user);
    if (kCommSucc != ret) {
        LOG_WARN("PlayerEnterGame failed.");
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int GameNetManager::OnLookerEnterGame(const REQUEST &request) {
    game::base::RS_UserEnterGameNotify ntf;
    const auto parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        LOG_WARN("ParseFromRequest failed.");
        ASSERT_FALSE_RETURN;
    }

    // 更新用户
    const auto& user_data = ntf.user();
    const auto userid = user_data.userid();
    const auto roomid = user_data.roomid();
    const auto tableno = user_data.tableno();
    const auto chairno = user_data.chairno();
    const auto user_type = user_data.user_type();
    const auto deposit = user_data.deposit();
    const auto total_bout = user_data.total_bout();
    const auto win_bout = user_data.win_bout();
    const auto loss_bout = user_data.loss_bout();
    const auto offline_count = user_data.offline_count();
    const auto& head_url = user_data.head_url();
    const auto& hardid = user_data.hardid();
    const auto& nick_name = user_data.nick_name();

    LOG_INFO("userid [%d] roomid [%d] tableno [%d] [%s]", userid, roomid, tableno, REQ_STR(request.head.nRequest));

    auto user = std::make_shared<User>();
    user->set_user_id(userid);
    user->set_room_id(roomid);
    user->set_table_no(tableno);
    user->set_chair_no(chairno);
    user->set_user_type(user_type);
    user->set_deposit(deposit);
    user->set_total_bout(total_bout);
    user->set_win_bout(win_bout);
    user->set_loss_bout(loss_bout);
    user->set_offline_count(offline_count);
    user->set_head_url(head_url);
    user->set_hard_id(hardid);
    user->set_nick_name(nick_name);

    if (kCommSucc != user_mgr_.AddUser(userid, user)) {
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != DepositDataMgr.SetDeposit(userid, deposit)) {
        ASSERT_FALSE_RETURN;
    }

    //更新房间
    const auto& room_data = ntf.room_data();
    RoomPtr room;
    if (kCommSucc != room_mgr_.GetRoom(room_data.roomid(), room)) {
        LOG_WARN("GetRoom failed room");
        ASSERT_FALSE_RETURN;
    }

    const auto options = room_data.options();
    const auto configs = room_data.configs();
    const auto manages = room_data.manages();
    const auto max_table_cout = room_data.max_table_cout();
    const auto chaircount_per_table = room_data.chaircount_per_table();
    const auto min_deposit = room_data.min_deposit();
    const auto max_deposit = room_data.max_deposit();
    const auto min_player_count = room_data.min_player_count();

    room->set_options(options);
    room->set_configs(configs);
    room->set_manages(manages);
    room->set_max_table_cout(max_table_cout);
    room->set_chaircount_per_table(chaircount_per_table);
    room->set_min_deposit(min_deposit);
    room->set_max_deposit(max_deposit);
    room->set_min_playercount_per_table(min_player_count);

    // 更新桌银 
    TablePtr table;
    if (kCommSucc != room->GetTable(tableno, table)) {
        LOG_WARN("GetTable faild. userid  [%d], tableno [%d]", userid, tableno);
        ASSERT_FALSE_RETURN;
    }
    table->set_max_deposit(ntf.max_deposit());
    table->set_min_deposit(ntf.min_deposit());
    table->set_base_deposit(ntf.base_deposit());

    const auto ret = room->LookerEnterGame(user); //Add User To User Manager
    if (kCommSucc != ret) {
        LOG_WARN("PlayerEnterGame failed.");
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int GameNetManager::OnLooker2Player(const REQUEST &request) const {
    game::base::RS_SwitchLookerPlayerNotify ntf;
    const auto parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        LOG_WARN("ParseFromRequest failed.");
        ASSERT_FALSE_RETURN;
    }

    const auto userid = ntf.userid();
    const auto roomid = ntf.roomid();
    const auto tableno = ntf.tableno();
    const auto chairno = ntf.chairno();
    assert(chairno != 0);
    LOG_INFO("userid [%d] roomid [%d] tableno [%d] chairno [%d] [%s]", userid, roomid, tableno, chairno, REQ_STR(request.head.nRequest));

    RoomPtr room;
    if (kCommSucc != room_mgr_.GetRoom(roomid, room)) {
        LOG_WARN("GetRoom failed room");
        ASSERT_FALSE_RETURN;
    }

    UserPtr user;
    if (kCommSucc != user_mgr_.GetUser(userid, user)) {
        ASSERT_FALSE_RETURN;
    }
    user->set_room_id(roomid);
    user->set_table_no(tableno);
    user->set_chair_no(chairno);

    const auto ret = room->Looker2Player(user);
    if (kCommSucc != ret) {
        LOG_WARN("Looker2Player failed");
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int GameNetManager::OnPlayer2Looker(const REQUEST &request) const {
    game::base::RS_SwitchLookerPlayerNotify ntf;
    const auto parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        LOG_WARN("ParseFromRequest failed.");
        ASSERT_FALSE_RETURN;
    }

    const auto userid = ntf.userid();
    const auto roomid = ntf.roomid();
    const auto tableno = ntf.tableno();
    const auto chairno = ntf.chairno();

    LOG_INFO("userid [%d] roomid [%d] tableno [%d] chairno [%d] [%s]", userid, roomid, tableno, chairno, REQ_STR(request.head.nRequest));

    RoomPtr room;
    if (kCommSucc != room_mgr_.GetRoom(roomid, room)) {
        LOG_WARN("GetRoom failed room");
        ASSERT_FALSE_RETURN;
    }

    UserPtr user;
    if (kCommSucc != user_mgr_.GetUser(userid, user)) {
        ASSERT_FALSE_RETURN;
    }
    user->set_room_id(roomid);
    user->set_table_no(tableno);
    user->set_chair_no(chairno);

    const auto ret = room->Player2Looker(user);
    if (kCommSucc != ret) {
        LOG_WARN("Player2Looker failed");
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int GameNetManager::OnStartGame(const REQUEST &request) const {
    game::base::RS_StartGameNotify ntf;
    const auto parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        LOG_WARN("ParseFromRequest failed.");
        ASSERT_FALSE_RETURN;
    }

    const auto roomid = ntf.roomid();
    const auto tableno = ntf.tableno();

    LOG_INFO("roomid [%d] tableno [%d] [%s]", roomid, tableno, REQ_STR(request.head.nRequest));

    RoomPtr room;
    if (kCommSucc != room_mgr_.GetRoom(roomid, room)) {
        LOG_WARN("GetRoom failed room");
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != room->StartGame(tableno)) {
        ASSERT_FALSE_RETURN;
    }

    //注意开局之后 可变桌椅还是有玩家可以上桌准备状态的
    return kCommSucc;
}

int GameNetManager::OnUserFreshResult(const REQUEST &request) const {
    game::base::RS_UserRefreshResultNotify ntf;
    const auto parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        LOG_WARN("ParseFromRequest failed.");
        ASSERT_FALSE_RETURN;
    }

    const auto userid = ntf.userid();
    const auto roomid = ntf.roomid();
    const auto tableno = ntf.tableno();

    LOG_INFO("userid [%d] roomid [%d] tableno [%d] [%s]", userid, roomid, tableno, REQ_STR(request.head.nRequest));

    RoomPtr room;
    if (kCommSucc != room_mgr_.GetRoom(roomid, room)) {
        LOG_WARN("GetRoom failed room");
        ASSERT_FALSE_RETURN;
    }
    TablePtr table;
    if (kCommSucc != room->GetTable(tableno, table)) {
        LOG_WARN("GetTable faild. tableno  [%d]", tableno);
        ASSERT_FALSE_RETURN;
    }

    table->RefreshGameResult(userid);
    return kCommSucc;
}

int GameNetManager::OnFreshResult(const REQUEST &request) const {
    game::base::RS_RefreshResultNotify ntf;
    const auto parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        LOG_WARN("ParseFromRequest failed.");
        ASSERT_FALSE_RETURN;
    }

    const auto roomid = ntf.roomid();
    const auto tableno = ntf.tableno();

    LOG_INFO("roomid [%d] tableno [%d] [%s]", roomid, tableno, REQ_STR(request.head.nRequest));

    RoomPtr room;
    if (kCommSucc != room_mgr_.GetRoom(roomid, room)) {
        LOG_WARN("GetRoom failed room");
        ASSERT_FALSE_RETURN;
    }

    TablePtr table;
    if (kCommSucc != room->GetTable(tableno, table)) {
        LOG_WARN("GetTable faild.  tableno  [%d]", tableno);
        ASSERT_FALSE_RETURN;
    }

    table->RefreshGameResult();
    return kCommSucc;
}

int GameNetManager::OnLeaveGame(const REQUEST &request) {
    game::base::RS_UserLeaveGameNotify ntf;
    const auto parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        LOG_WARN("ParseFromRequest failed.");
        ASSERT_FALSE_RETURN;
    }

    // 更新用户
    const auto& user_data = ntf.user();
    const auto userid = user_data.userid();
    const auto roomid = user_data.roomid();
    const auto tableno = user_data.tableno();
    const auto chairno = user_data.chairno();
    const auto user_type = user_data.user_type();
    const auto deposit = user_data.deposit();
    const auto total_bout = user_data.total_bout();
    const auto win_bout = user_data.win_bout();
    const auto loss_bout = user_data.loss_bout();
    const auto offline_count = user_data.offline_count();
    const auto& head_url = user_data.head_url();
    const auto& hardid = user_data.hardid();
    const auto& nick_name = user_data.nick_name();

    LOG_INFO("userid [%d] roomid [%d] tableno [%d] chairno [%d] [%s]", userid, roomid, tableno, chairno, REQ_STR(request.head.nRequest));

    auto user = std::make_shared<User>();
    user->set_user_id(userid);
    user->set_room_id(roomid);
    user->set_table_no(tableno);
    user->set_chair_no(chairno);
    user->set_user_type(user_type);
    user->set_deposit(deposit);
    user->set_total_bout(total_bout);
    user->set_win_bout(win_bout);
    user->set_loss_bout(loss_bout);
    user->set_offline_count(offline_count);
    user->set_head_url(head_url);
    user->set_hard_id(hardid);
    user->set_nick_name(nick_name);

    RoomPtr room;
    if (kCommSucc != room_mgr_.GetRoom(roomid, room)) {
        LOG_WARN("GetRoom failed roomid [%d]", roomid);
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != DepositDataMgr.SetDeposit(userid, deposit)) {
        ASSERT_FALSE_RETURN;
    }

    if (!room)ASSERT_FALSE_RETURN;
    room->UserLeaveGame(userid, tableno);

    if (kCommSucc != user_mgr_.DelUser(userid)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int GameNetManager::OnSwitchTable(const REQUEST &request) const {
    game::base::RS_SwitchTableNotify ntf;
    const auto parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        LOG_WARN("ParseFromRequest failed.");
        ASSERT_FALSE_RETURN;
    }

    const auto userid = ntf.userid();
    const auto roomid = ntf.roomid();
    const auto old_tableno = ntf.old_tableno();
    const auto new_tableno = ntf.new_tableno();
    const auto new_chairno = ntf.new_chairno();

    LOG_INFO("userid [%d] roomid [%d] old_tableno [%d] new_tableno [%d] [%s]", userid, roomid, old_tableno, new_tableno, REQ_STR(request.head.nRequest));

    UserPtr user;
    if (kCommSucc != user_mgr_.GetUser(userid, user)) {
        ASSERT_FALSE_RETURN;
    }
    user->set_room_id(roomid);
    user->set_table_no(new_tableno);
    user->set_chair_no(new_chairno);

    RoomPtr room;
    if (kCommSucc != room_mgr_.GetRoom(roomid, room)) {
        LOG_WARN("GetRoom failed room");
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != room->SwitchTable(user, old_tableno)) {
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;

}

int GameNetManager::OnNewRoom(const REQUEST &request) {
    game::base::RS_NewRoomNotify ntf;
    const auto parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        LOG_WARN("ParseFromRequest failed.");
        ASSERT_FALSE_RETURN;
    }

    const auto room_pb = ntf.room();
    const auto& room_data_pb = room_pb.room_data();
    const auto roomid = room_data_pb.roomid();
    LOG_INFO("roomid [%d] [%s]", roomid, REQ_STR(request.head.nRequest));

    if (kCommSucc != room_mgr_.AddRoomPB(room_pb)) {
        ASSERT_FALSE_RETURN
    }


    return kCommSucc;
}

int GameNetManager::ConnectWithLock(const std::string& game_ip, const int& game_port) const {
    CHECK_GAMEIP(game_ip);
    CHECK_GAMEPORT(game_port);
    CHECK_CONNECTION(connection_);
    LOG_INFO("\t[START] Try ConnectGame ... IP: [%s] Port: [%d]", game_ip.c_str(), game_port);
    connection_->InitKey(KEY_GAMESVR_2_0, ENCRYPT_AES, 0);
    if (!connection_->Create(game_ip.c_str(), game_port, 5, 0, notify_thread_.GetThreadID(), 0, GetHelloData(), GetHelloLength())) {
        LOG_ERROR("\t[START] ConnectGame Faild! IP: [%s] Port: [%d]", game_ip.c_str(), game_port);
        EVENT_TRACK(EventType::kErr, kCreateGameConnFailed);
        return kCommFaild;
    }

    LOG_INFO("game manager connect ok ! token [%d] ip [%s] port: [%d]", connection_->GetTokenID(), game_ip.c_str(), game_port);
    return kCommSucc;
}

// 游戏数据层是否初始化完毕  允许脏读
bool GameNetManager::IsGameDataInited() const {
    if (!connection_->IsConnected()) {
        return false;
    }
    return game_data_inited_;
}

int GameNetManager::GetTableDepositRange(const RoomID& roomid, const TableNO& tableno, int64_t& max, int64_t& min) {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    return room_mgr_.GetTableDepositRange(roomid, tableno, max, min);
}

int GameNetManager::GetRoomDepositRange(int64_t& max, int64_t& min) const {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    return room_mgr_.GetRoomDepositRange(max, min);
}

int GameNetManager::GetRobotCountOnChairs(const RoomID& roomid, const TableNO& tableno, int& count) const {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    return room_mgr_.GetRobotCountOnChairs(roomid, tableno, count);
}

int GameNetManager::GetChairInfo(const RoomID& roomid, const TableNO& tableno, const int& userid, ChairInfo& info) const {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    return room_mgr_.GetChairInfo(roomid, tableno, userid, info);
}

int GameNetManager::IsTablePlaying(const RoomID& roomid, const TableNO& tableno, bool& result) const {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    return room_mgr_.IsTablePlaying(roomid, tableno, result);
}

int GameNetManager::GetNormalCountOnChairs(const RoomID& roomid, const TableNO& tableno, int& count) const {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    return room_mgr_.GetNormalCountOnChairs(roomid, tableno, count);
}

int GameNetManager::GetMiniPlayers(const RoomID& roomid, int& mini) const {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    return room_mgr_.GetMiniPlayers(roomid, mini);
}

int GameNetManager::IsRobotUserExist(const UserID& userid) const {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    return user_mgr_.IsRobotUserExist(userid);
}

int GameNetManager::GetNormalUserMap(UserMap& normal_user_map) const {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    return user_mgr_.GetNormalUserMap(normal_user_map);
}


UserMap GameNetManager::GetAllUsers() const {
    ThreadID thread_id = GetCurrentThreadId();
    if (thread_id != g_launchThreadID && thread_id != g_mainThreadID) {
        ASSERT_FALSE;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    return user_mgr_.GetAllUsers();
}

int GameNetManager::SnapShotObjectStatus() const {
    CHECK_MAIN_OR_LAUNCH_THREAD();
#ifdef _DEBUG
    std::lock_guard<std::mutex> lock(mutex_);
    if (connection_) {
        LOG_INFO("token = %x", connection_->GetTokenID());
    }
    LOG_INFO("game ip [%s]", game_ip_.c_str());
    LOG_INFO("game port [%d]", game_port_);

    LOG_INFO("game_info_notify_thread_ [%d]", notify_thread_.GetThreadID());
    LOG_INFO("heart_timer_thread_ [%d]", timer_thread_.GetThreadID());
    LOG_INFO("token [%d]", connection_->GetTokenID());
#endif
    return kCommSucc;
}


int GameNetManager::BriefInfo() const {
    return user_mgr_.BriefInfo();
}

int GameNetManager::IsConnected(BOOL& is_connected) const {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    if (!connection_) {
        is_connected = false;
        return kCommSucc;
    }
    is_connected = connection_->IsConnected();
    return kCommSucc;
}

