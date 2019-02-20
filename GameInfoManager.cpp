#include "stdafx.h"
#include "Main.h"
#include "common_func.h"
#include "GameInfoManager.h"
#include "RobotUitls.h"


bool	GameInfoManager::Init() {
    if (kCommFaild == InitConnectGame()) {
        UWL_ERR("InitConnectGame failed");
        assert(false);
        return false;
    }
    m_thrdGameInfoNotify.Initial(std::thread([this] {this->ThreadRunGameInfoNotify(); }));
    return true;
}

void	GameInfoManager::Term() {
    m_thrdGameInfoNotify.Release();
}

int GameInfoManager::GetUserStatus(UserID userid, UserStatus& user_status) {
    std::lock_guard<std::mutex> lock(game_info_connection_mutex_);

    if (user_map_.find(userid) == user_map_.end()) return kCommFaild;
    auto user = user_map_[userid];
    auto chairno = user.chairno();

    // 玩家信息中token为0则说明玩家离线； kUserOffline = 0x10000000		// 断线
    // TODO

    // 玩家信息中椅子号为0则说明在旁观；
    if (chairno == 0) {
        user_status = kUserLooking;
        return kCommSucc;
    }

    // 有椅子号则查看桌子状态，桌子waiting -> 玩家waiting
    game::base::Table table;
    if (kCommFaild == FindTable(userid, table)) return kCommFaild;
    auto table_status = table.table_status();
    if (table_status == kTableWaiting) {
        user_status = kUserWaiting;
        return kCommSucc;
    }

    // 桌子playing && 椅子playing -> 玩家playing
    game::base::ChairInfo chair;
    if (kCommFaild == FindChair(userid, chair)) return kCommFaild;
    auto chair_status = chair.chair_status();
    if (table_status != kTablePlaying) return kCommFaild;

    if (chair_status == kChairPlaying) {
        user_status = kUserPlaying;
        return kCommSucc;
    }

    // 桌子playing && 椅子waiting -> 等待下局游戏开始（原空闲玩家）//TODO 原空闲玩家?
    if (chair_status == kChairWaiting) {
        user_status = kUserWaiting;
        return kCommSucc;
    }

    return kCommFaild;
}



int GameInfoManager::FindTable(UserID userid, game::base::Table& table) {
    if (user_map_.find(userid) == user_map_.end()) return kCommFaild;
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
    }
    return kCommFaild;
}


int GameInfoManager::FindChair(UserID userid, game::base::ChairInfo& chair) {
    if (user_map_.find(userid) == user_map_.end()) return kCommFaild;
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
    }

    return kCommFaild;
}

int GameInfoManager::InitConnectGame() {
    TCHAR szGameSvrIP[MAX_SERVERIP_LEN] = {};
    GetPrivateProfileString(_T("GameServer"), _T("IP"), _T(""), szGameSvrIP, sizeof(szGameSvrIP), g_szIniFile);
    auto nGameSvrPort = GetPrivateProfileInt(_T("GameServer"), _T("Port"), 0, g_szIniFile);

    {
        std::lock_guard<std::mutex> lock(game_info_connection_mutex_);
        game_info_connection_->InitKey(KEY_GAMESVR_2_0, ENCRYPT_AES, 0);

        if (!game_info_connection_->Create(szGameSvrIP, nGameSvrPort, 5, 0, m_thrdGameInfoNotify.ThreadId(), 0, GetHelloData(), GetHelloLength())) {
            UWL_ERR("[ROUTE] ConnectGame Faild! IP:%s Port:%d", szGameSvrIP, nGameSvrPort);
            assert(false);
            return kCommFaild;
        }
        UWL_INF("ConnectGame OK! IP:%s Port:%d", szGameSvrIP, nGameSvrPort);

        auto ret = SendValidateReq();

        if (ret == kCommFaild) return ret;

        UWL_INF("SendValidateReq OK!");

        ret = SendGetGameInfo();

        if (ret == kCommFaild) return ret;

        UWL_INF("SendGetGameInfo OK!");

    }


    return kCommSucc;
}

int GameInfoManager::SendValidateReq() {
    std::lock_guard<std::mutex> lock(game_info_connection_mutex_);

    game::base::RobotSvrValidateReq req;
    req.set_client_id(g_nClientID);
    REQUEST response = {};
    auto result = RobotUitls::SendRequest(game_info_connection_, GR_VALID_ROBOTSVR, req, response);

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
        UWL_ERR("SendValidateReq faild. check return[%d]. req = %s", resp.code(), GetStringFromPb(req).c_str());
        return kCommFaild;
    }

    return kCommSucc;
}

// 机器人服务获取游戏内所有玩家信息(room、table、user)
int GameInfoManager::SendGetGameInfo(RoomID roomid /*= 0*/) {
    std::lock_guard<std::mutex> lock(game_info_connection_mutex_);

    game::base::GetGameUsersReq req;
    req.set_clientid(g_nClientID);
    req.set_roomid(roomid);
    REQUEST response = {};
    auto result = RobotUitls::SendRequest(game_info_connection_, GR_GET_GAMEUSERS, req, response);

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
        UWL_ERR("SendGetGameInfo faild. check return[%d]. req = %s", resp.code(), GetStringFromPb(req).c_str());
        return kCommFaild;
    }

    room_map_.clear();
    for (int index = 0; index < resp.rooms_size(); index++) {
        game::base::Room room = resp.rooms(index);
        RoomID roomid = room.room_data().roomid();
        room_map_.insert(std::make_pair(roomid, room));
    }

    user_map_.clear();
    for (int index = 0; index < resp.users_size(); index++) {
        game::base::User user = resp.users(index);
        UserID userid = user.userid();
        user_map_.insert(std::make_pair(userid, user));
    }
    return kCommSucc;
}


void GameInfoManager::OnDisconnGameInfo() {
    game_info_connection_->DestroyEx();
}


void	GameInfoManager::ThreadRunGameInfoNotify() {
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

        case GN_USER_STATUS_TO_ROBOTSVR:
        {
            OnRecvGameInfo(request);
        }
        break;
    }

}

void GameInfoManager::OnRecvGameInfo(const REQUEST &request) {
    game::base::Status2RobotSvrNotify user_status_ntf;
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

}
