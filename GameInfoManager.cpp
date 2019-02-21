#include "stdafx.h"
#include "Main.h"
#include "common_func.h"
#include "GameInfoManager.h"
#include "RobotUitls.h"
#include "base_room.h"
#include "user.h"
#include "roommgr.h"
#include "usermgr.h"


int GameInfoManager::Init() {
    if (kCommFaild == ConnectGame()) {
        UWL_ERR("ConnectGame failed");
        assert(false);
        return kCommFaild;
    }

    if (kCommFaild == SendValidateReq()) {
        UWL_ERR("SendValidateReq failed");
        assert(false);
        return kCommFaild;
    }

    if (kCommFaild == SendGetGameInfo()) {
        UWL_ERR("SendGetGameInfo failed");
        assert(false);
        return kCommFaild;
    }

    game_info_notify_thread_.Initial(std::thread([this] {this->ThreadGameInfoNotify(); }));

    heart_timer_thread_.Initial(std::thread([this] {this->ThreadSendGamePluse(); }));

    return kCommSucc;
}

void GameInfoManager::Term() {
    game_info_notify_thread_.Release();
    heart_timer_thread_.Release();
}

int GameInfoManager::ConnectGame() {
    TCHAR szGameSvrIP[MAX_SERVERIP_LEN] = {};
    GetPrivateProfileString(_T("GameServer"), _T("IP"), _T(""), szGameSvrIP, sizeof(szGameSvrIP), g_szIniFile);
    auto nGameSvrPort = GetPrivateProfileInt(_T("GameServer"), _T("Port"), 0, g_szIniFile);

    {
        std::lock_guard<std::mutex> lock(game_info_connection_mutex_);
        game_info_connection_->InitKey(KEY_GAMESVR_2_0, ENCRYPT_AES, 0);
        if (!game_info_connection_->Create(szGameSvrIP, nGameSvrPort, 5, 0, game_info_notify_thread_.ThreadId(), 0, GetHelloData(), GetHelloLength())) {
            UWL_ERR("[ROUTE] ConnectGame Faild! IP:%s Port:%d", szGameSvrIP, nGameSvrPort);
            assert(false);
            return kCommFaild;
        }
    }

    UWL_INF("ConnectGame OK! IP:%s Port:%d", szGameSvrIP, nGameSvrPort);
    return kCommSucc;
}

int GameInfoManager::SendGameRequest(RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool bNeedEcho /*= true*/) {
    std::lock_guard<std::mutex> lock(game_info_connection_mutex_);
    return RobotUitls::SendRequest(game_info_connection_, requestid, val, response, bNeedEcho);
}

void GameInfoManager::ThreadGameInfoNotify() {
    UWL_INF(_T("GameNotify thread started. id = %d"), GetCurrentThreadId());

    MSG msg = {};
    while (GetMessage(&msg, 0, 0, 0)) {
        if (UM_DATA_RECEIVED == msg.message) {

            LPCONTEXT_HEAD	pContext = (LPCONTEXT_HEAD) (msg.wParam);
            LPREQUEST		pRequest = (LPREQUEST) (msg.lParam);

            TokenID		nTokenID = pContext->lTokenID;
            RequestID		nReqstID = pRequest->head.nRequest;

            OnGameInfoNotify(nReqstID, *pRequest);

            SAFE_DELETE(pContext);
            UwlClearRequest(pRequest);
            SAFE_DELETE(pRequest);
        } else {
            DispatchMessage(&msg);
        }
    }
    UWL_INF(_T("GameNotify thread exiting. id = %d"), GetCurrentThreadId());
    return;
}

void GameInfoManager::OnGameInfoNotify(RequestID nReqstID, const REQUEST &request) {

    switch (nReqstID) {
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
            OnSwitchGame(request);
            break;
        default:
            break;


    }
}

void GameInfoManager::OnDisconnGameInfo() {
    std::lock_guard<std::mutex> lock(game_info_connection_mutex_);
    if (game_info_connection_)
        game_info_connection_->DestroyEx();
}

void GameInfoManager::ThreadSendGamePluse() {
    UWL_INF(_T("Game KeepAlive thread started. id = %d"), GetCurrentThreadId());
    while (TRUE) {
        DWORD dwRet = WaitForSingleObject(g_hExitServer, PluseInterval);
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }

        if (WAIT_TIMEOUT == dwRet) {

            std::lock_guard<std::mutex> lock(game_info_connection_mutex_);

            game::base::PulseReq val;
            val.set_id(g_nClientID);
            REQUEST response = {};
            auto result = RobotUitls::SendRequest(game_info_connection_, GR_GAME_PLUSE, val, response, false);

            if (kCommSucc != result) {
                UWL_ERR("Send game pluse failed");
            }

        }
    }
    UWL_INF(_T("Game KeepAlive thread exiting. id = %d"), GetCurrentThreadId());
    return;
}

// 具体业务
int GameInfoManager::SendValidateReq() {

    game::base::RobotSvrValidateReq val;
    val.set_client_id(g_nClientID);
    REQUEST response = {};
    auto result = SendGameRequest(GR_VALID_ROBOTSVR, val, response);

    if (kCommSucc != result) {
        UWL_ERR("SendValidateReq failed");
        return kCommFaild;
    }

    game::base::RobotSvrValidateResp resp;
    int ret = ParseFromRequest(response, resp);
    if (kCommSucc != ret) {
        UWL_ERR("ParseFromRequest faild.");
        return kCommFaild;
    }

    if (kCommSucc != resp.code()) {
        UWL_ERR("SendValidateReq faild. check return[%d]. req = %s", resp.code(), GetStringFromPb(val).c_str());
        return kCommFaild;
    }

    return kCommSucc;
}

int GameInfoManager::SendGetGameInfo(RoomID roomid /*= 0*/) {
    game::base::GetGameUsersReq val;
    val.set_clientid(g_nClientID);
    val.set_roomid(roomid);
    REQUEST response = {};
    auto result = SendGameRequest(GR_GET_GAMEUSERS, val, response);
    if (kCommSucc != result) {
        UWL_ERR("SendGetGameInfo failed");
        return kCommFaild;
    }

    game::base::GetGameUsersResp resp;
    int ret = ParseFromRequest(response, resp);
    if (kCommSucc != ret) {
        UWL_ERR("ParseFromRequest faild.");
        return kCommFaild;
    }

    if (kCommSucc != resp.code()) {
        UWL_ERR("SendGetGameInfo faild. check return[%d]. req = %s", resp.code(), GetStringFromPb(val).c_str());
        return kCommFaild;
    }

    RoomMgr::Instance().Reset();
    for (int room_index = 0; room_index < resp.rooms_size(); room_index++) {
        game::base::Room room_pb = resp.rooms(room_index);
        AddRoom(room_pb);
    }

    UserMgr::Instance().Reset();
    for (int user_index = 0; user_index < resp.users_size(); user_index++) {
        game::base::User user_pb = resp.users(user_index);
        AddUser(user_pb);
    }

    return kCommSucc;
}

void GameInfoManager::OnRecvGameStatus(const REQUEST &request) {
    /*game::base::Status2RobotSvrNotify user_status_ntf;
    int parse_ret = ParseFromRequest(request, user_status_ntf);
    if (kCommSucc != parse_ret) {
    UWL_WRN("ParseFromRequest failed.");
    return;
    }

    UserID userid = user_status_ntf.userid();
    if (user_map_.find(userid) == user_map_.end()) return;

    auto user = user_map_[userid];
    user.set_userid(user_status_ntf.userid());
    user.set_roomid(user_status_ntf.roomid());
    user.set_tableno(user_status_ntf.tableno());
    user.set_chairno(user_status_ntf.chairno());
    user.set_user_type(user_status_ntf.user_type());
    */
}


void GameInfoManager::OnPlayerEnterGame(const REQUEST &request) {
    game::base::RS_UserEnterGameNotify ntf;
    int parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        assert(false);
        UWL_WRN("ParseFromRequest failed.");
        return;
    }

    game::base::RoomData room_data = ntf.room_data();
    auto roomid = room_data.roomid();

    std::shared_ptr<User> user;
    user->set_user_id(ntf.userid());
    user->set_table_no(ntf.tableno());
    user->set_chair_no(ntf.chairno());
    user->set_user_type(ntf.user_type());
    user->set_room_id(roomid);

    auto base_room = RoomMgr::Instance().GetRoom(roomid);
    if (!base_room) {
        assert(false);
        UWL_WRN("GetRoom failed room");
        return;
    }
    int ret = base_room->PlayerEnterGame(user); //Add User Manager
    if (ret != kCommSucc) {
        assert(false);
        UWL_WRN("PlayerEnterGame failed.");
        return;
    }
}

void GameInfoManager::OnLookerEnterGame(const REQUEST &request) {
    game::base::RS_UserEnterGameNotify ntf;
    int parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        assert(false);
        UWL_WRN("ParseFromRequest failed.");
        return;
    }

    game::base::RoomData room_data = ntf.room_data();
    auto roomid = room_data.roomid();

    std::shared_ptr<User> user;
    user->set_user_id(ntf.userid());
    user->set_room_id(roomid);
    user->set_table_no(ntf.tableno());
    user->set_chair_no(ntf.chairno());
    user->set_user_type(ntf.user_type());

    auto base_room = RoomMgr::Instance().GetRoom(roomid);
    if (!base_room) {
        assert(false);
        UWL_WRN("GetRoom failed room");
        return;
    }

    int ret = base_room->LookerEnterGame(user); //Add User To User Manager
    if (ret != kCommSucc) {
        UWL_WRN("PlayerEnterGame failed.");
        return;
    }
}

void GameInfoManager::OnLooker2Player(const REQUEST &request) {
    game::base::RS_SwitchLookerPlayerNotify ntf;
    int parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        assert(false);
        UWL_WRN("ParseFromRequest failed.");
        return;
    }

    auto userid = ntf.userid();
    auto roomid = ntf.roomid();
    auto tableno = ntf.tableno();
    auto chairno = ntf.chairno();

    auto base_room = RoomMgr::Instance().GetRoom(roomid);
    if (!base_room) {
        assert(false);
        UWL_WRN("GetRoom failed room");
        return;
    }

    auto user = UserMgr::Instance().GetUser(userid);
    user->set_room_id(roomid);
    user->set_table_no(tableno);
    user->set_chair_no(chairno);

    int ret = base_room->Looker2Player(user);
    if (ret != kCommSucc) {
        assert(false);
        UWL_WRN("Looker2Player failed");
        return;
    }

}

void GameInfoManager::OnPlayer2Looker(const REQUEST &request) {
    game::base::RS_SwitchLookerPlayerNotify ntf;
    int parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        assert(false);
        UWL_WRN("ParseFromRequest failed.");
        return;
    }

    auto userid = ntf.userid();
    auto roomid = ntf.roomid();
    auto tableno = ntf.tableno();
    auto chairno = ntf.chairno();

    auto base_room = RoomMgr::Instance().GetRoom(roomid);
    if (!base_room) {
        assert(false);
        UWL_WRN("GetRoom failed room");
        return;
    }

    auto user = UserMgr::Instance().GetUser(userid);
    user->set_room_id(roomid);
    user->set_table_no(tableno);
    user->set_chair_no(chairno);

    int ret = base_room->Player2Looker(user);
    if (ret != kCommSucc) {
        assert(false);
        UWL_WRN("Player2Looker failed");
        return;
    }

}

void GameInfoManager::OnStartGame(const REQUEST &request) {

}

void GameInfoManager::OnUserFreshResult(const REQUEST &request) {

}

void GameInfoManager::OnFreshResult(const REQUEST &request) {

}

void GameInfoManager::OnLeaveGame(const REQUEST &request) {

}

void GameInfoManager::OnSwitchGame(const REQUEST &request) {

}

int GameInfoManager::GetUserStatus(UserID userid, UserStatus& user_status) {
    //std::lock_guard<std::mutex> lock(game_info_connection_mutex_);

    //if (user_map_.find(userid) == user_map_.end()) return kCommFaild;
    //auto user = user_map_[userid];
    //auto chairno = user.chairno();

    //// 玩家信息中token为0则说明玩家离线； kUserOffline = 0x10000000		// 断线
    //// TODO

    //// 玩家信息中椅子号为0则说明在旁观；
    //if (chairno == 0) {
    //    user_status = kUserLooking;
    //    return kCommSucc;
    //}

    //// 有椅子号则查看桌子状态，桌子waiting -> 玩家waiting
    //game::base::Table table;
    //if (kCommFaild == FindTable(userid, table)) return kCommFaild;
    //auto table_status = table.table_status();
    //if (table_status == kTableWaiting) {
    //    user_status = kUserWaiting;
    //    return kCommSucc;
    //}

    //// 桌子playing && 椅子playing -> 玩家playing
    //game::base::ChairInfo chair;
    //if (kCommFaild == FindChair(userid, chair)) return kCommFaild;
    //auto chair_status = chair.chair_status();
    //if (table_status != kTablePlaying) return kCommFaild;

    //if (chair_status == kChairPlaying) {
    //    user_status = kUserPlaying;
    //    return kCommSucc;
    //}

    //// 桌子playing && 椅子waiting -> 等待下局游戏开始（原空闲玩家）//TODO 原空闲玩家?
    //if (chair_status == kChairWaiting) {
    //    user_status = kUserWaiting;
    //    return kCommSucc;
    //}

    return kCommFaild;
}

int GameInfoManager::FindTable(UserID userid, game::base::Table& table) {
    /* if (user_map_.find(userid) == user_map_.end()) return kCommFaild;
     auto user = user_map_[userid];
     auto tableno = user.tableno();

     auto roomid = user.roomid();
     if (room_map_.find(roomid) == room_map_.end()) return kCommFaild;

     auto tables = room_map_[roomid].tables();

     for (auto& table : tables) {
     if (table.tableno() == tableno) {
     table = tables[tableno];
     return kCommSucc;
     }
     }*/
    return kCommFaild;
}

int GameInfoManager::FindChair(UserID userid, game::base::ChairInfo& chair) {
    /* if (user_map_.find(userid) == user_map_.end()) return kCommFaild;
     auto user = user_map_[userid];
     auto chairno = user.chairno();

     game::base::Table table;
     if (kCommFaild == FindTable(userid, table)) return kCommFaild;

     auto chairs = table.chairs();

     for (auto& chair : chairs) {
     if (chair.chairno() == chairno) {
     chair = chairs[chairno];
     return kCommSucc;
     }
     }*/

    return kCommFaild;
}

int GameInfoManager::AddRoom(game::base::Room room_pb) {
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

        //TABLE USER INFO
        for (int table_user_index = 0; table_user_index < table_pb.table_users_size(); table_user_index++) {
            game::base::TableUserInfo table_user_pb = table_pb.table_users(table_user_index);
            UserID userid = table_user_pb.userid();
            TableUserInfo table_user_info;
            table_user_info.set_userid(table_user_pb.userid());
            table_user_info.set_bind_timestamp(table_user_pb.bind_timestamp());
            table->AddTableUserInfo(userid, table_user_info);
        }

        base_room->AddTable(tableno, table);
    }
    RoomMgr::Instance().AddRoom(roomid, base_room);
    return kCommSucc;
}

int GameInfoManager::AddUser(game::base::User user_pb) {
    UserID userid = user_pb.userid();
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
    UserMgr::Instance().AddUser(userid, user);
    return kCommSucc;
}


