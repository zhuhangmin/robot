#include "stdafx.h"
#include "RobotMgr.h"
#include "Main.h"
#include "RobotReq.h"
#include "common_func.h"
#include "RobotUitls.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#define ROBOT_APPLY_DEPOSIT_KEY "zjPUYq9L36oA9zke"

bool	CRobotMgr::Init() {
    if (!InitSetting()) {
        UWL_ERR("InitSetting() return false");
        assert(false);
        return false;
    }
    UWL_INF("InitSetting Account Count = %d", account_setting_map_.size());

    if (!InitEnterGameThreads()) {
        UWL_ERR("InitEnterGameThreads() return false");
        assert(false);
        return false;
    }
    UWL_INF("InitEnterGameThreads Sucessed");

    if (!InitNotifyThreads()) {
        UWL_ERR("InitNotifyThreads() return false");
        assert(false);
        return false;
    }
    UWL_INF("InitNotifyThreads Sucessed");

    if (!InitConnectHall()) {
        UWL_ERR("ConnectHall() return false");
        assert(false);
        return false;
    }
    UWL_INF("InitConnectHall Sucessed.");

    /*if (!InitGameRoomDatas()) {
        UWL_ERR("InitGameRoomDatas() return false");
        assert(false);
        return false;
        }*/

    if (kCommFaild == InitConnectGame()) {
        UWL_ERR("InitConnectGame failed");
        assert(false);
        return false;
    }

    return true;
}
void	CRobotMgr::Term() {
    for (auto&& it : m_thrdEnterGames)
        it.Release();

    m_thrdHallNotify.Release();
    m_thrdRoomNotify.Release();
    m_thrdGameNotify.Release();
    m_thrdGameInfoNotify.Release();
}

bool CRobotMgr::InitSetting() {
    std::string filename = g_curExePath + ROBOT_SETTING_FILE_NAME;
    Json::Value root;
    Json::Reader reader;
    std::ifstream ifile;
    ifile.open(filename, std::ifstream::in);

    auto closeRet = [&ifile] (bool ret) {ifile.close(); return ret; };

    if (!reader.parse(ifile, root, FALSE))		return closeRet(false);

    if (!root.isMember("GameId"))               return closeRet(false);

    if (!root["RoomIds"].isArray())				return closeRet(false);

    if (!root["robots"].isArray())				return closeRet(false);

    g_gameID = root["GameId"].asInt();

    auto rooms = root["RoomIds"];
    int size = rooms.size();

    for (int n = 0; n < size; ++n) {
        int32_t nRoomId = rooms[n]["RoomId"].asInt();
        int32_t nCtrlMode = rooms[n]["CtrlMode"].asInt();
        int32_t nCtrlValue = rooms[n]["Value"].asInt();
        room_setting_map_[nRoomId] = RoomSetiing{nRoomId, nCtrlMode, nCtrlValue};
        room_cur_users_[nRoomId] = 0;
    }

    auto robots = root["robots"];
    size = robots.size();
    for (int i = 0; i < size; ++i) {
        auto robot = robots[i];
        int32_t		nAccount = robot["Account"].asInt();
        std::string sPassword = robot["Password"].asString();
        std::string sNickName = robot["NickName"].asString();
        std::string sPortrait = robot["Portrait"].asString();

        RobotSetting unit;
        unit.account = nAccount;
        unit.password = sPassword;
        unit.nickName = sNickName;
        unit.portraitUrl = sPortrait;
        account_setting_map_[nAccount] = unit;
    }
    return closeRet(true);
}
bool	CRobotMgr::InitNotifyThreads() {
    m_thrdHallNotify.Initial(std::thread([this] {this->ThreadRunHallNotify(); }));
    m_thrdRoomNotify.Initial(std::thread([this] {this->ThreadRunRoomNotify(); }));
    m_thrdGameNotify.Initial(std::thread([this] {this->ThreadRunGameNotify(); }));
    m_thrdGameInfoNotify.Initial(std::thread([this] {this->ThreadRunGameNotify(); }));
    return true;
}
bool	CRobotMgr::InitEnterGameThreads() {
    for (size_t i = 0; i < DEF_ENTER_GAME_THREAD_NUM; i++) {
        m_thrdEnterGames[i].Initial(std::thread([this] {this->ThreadRunEnterGame(); }));
    }
    return true;
}
bool	CRobotMgr::InitConnectHall(bool bReconn) {
    TCHAR szHallSvrIP[MAX_SERVERIP_LEN] = {};
    GetPrivateProfileString(_T("HallServer"), _T("IP"), _T(""), szHallSvrIP, sizeof(szHallSvrIP), g_szIniFile);
    auto nHallSvrPort = GetPrivateProfileInt(_T("HallServer"), _T("Port"), 0, g_szIniFile);

    {
        std::lock_guard<std::mutex> lock(hall_connection_mutex_);
        hall_connection_->InitKey(KEY_HALL, ENCRYPT_AES, 0);

        if (!hall_connection_->Create(szHallSvrIP, nHallSvrPort, 5, 0, m_thrdHallNotify.ThreadId(), 0, GetHelloData(), GetHelloLength())) {
            UWL_ERR("[ROUTE] ConnectHall Faild! IP:%s Port:%d", szHallSvrIP, nHallSvrPort);
            assert(false);
            return false;
        }
    }
    UWL_INF("ConnectHall[ReConn:%d] OK! IP:%s Port:%d", (int) bReconn, szHallSvrIP, nHallSvrPort);
    return true;
}

int CRobotMgr::InitConnectGame() {
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

//bool	CRobotMgr::InitGameRoomDatas() {
//    for (auto& itr : room_setting_map_) {
//        auto&& tRet = SendGetRoomData(itr.first);
//        if (!TUPLE_ELE(tRet, 0)) {
//            UWL_ERR("SendGetRoomData Faild! nRoomId:%d err:%s", itr.first, TUPLE_ELE_C(tRet, 1));
//            assert(false);
//        }
//    }
//    return true;
//}

int CRobotMgr::SendValidateReq() {
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
int CRobotMgr::SendGetGameInfo(RoomID roomid /*= 0*/) {
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

    return kCommSucc;
}

TTueRet CRobotMgr::SendHallRequest(RequestID nReqId, uint32_t& nDataLen, void *pData, RequestID &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho /*= true*/, uint32_t wait_ms /*= REQ_TIMEOUT_INTERVAL*/) {
    if (!hall_connection_) {
        UWL_ERR("SendHallRequest m_CoonHall nil ERR_CONNECT_NOT_EXIST nReqId = %d", nReqId);
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_NOT_EXIST);
    }

    if (!hall_connection_->IsConnected()) {
        UWL_ERR("SendHallRequest m_CoonHall not connect ERR_CONNECT_DISABLE nReqId = %d", nReqId);
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_DISABLE);
    }

    CONTEXT_HEAD	Context = {};
    REQUEST			Request = {};
    REQUEST			Response = {};
    Context.hSocket = hall_connection_->GetSocket();
    Context.lSession = 0;
    Context.bNeedEcho = bNeedEcho;
    Request.head.nRepeated = 0;
    Request.head.nRequest = nReqId;
    Request.nDataLen = nDataLen;
    Request.pDataPtr = pData;//////////////

    BOOL bTimeOut = FALSE, bResult = TRUE;
    {
        std::lock_guard<std::mutex> lock(hall_connection_mutex_);
        bResult = hall_connection_->SendRequest(&Context, &Request, &Response, bTimeOut, wait_ms);
    }

    if (!bResult)///if timeout or disconnect 
    {
        UWL_ERR("SendHallRequest m_ConnHall->SendRequest fail bTimeOut = %d, nReqId = %d", bTimeOut, nReqId);
        assert(false);
        return std::make_tuple(false, (bTimeOut ? ERR_SENDREQUEST_TIMEOUT : ERR_OPERATE_FAILED));
    }

    nDataLen = Response.nDataLen;
    nRespId = Response.head.nRequest;
    pRetData.reset(Response.pDataPtr);

    if (nRespId == GR_ERROR_INFOMATION) {
        CHAR info[512] = {};
        sprintf_s(info, "%s", pRetData);
        nDataLen = 0;
        UWL_ERR("SendHallRequest m_ConnHall->SendRequest fail nRespId GR_ERROR_INFOMATION nReqId = %d", nReqId);
        assert(false);
        return std::make_tuple(false, info);
    }
    return std::make_tuple(true, ERR_OPERATE_SUCESS);
}

bool CRobotMgr::GetRobotSetting(int account, RobotSetting& unit) {
    if (account == 0)	return false;

    auto it = account_setting_map_.find(account);
    if (it == account_setting_map_.end()) return false;

    unit = it->second;

    return true;
}
bool	CRobotMgr::GetRoomData(int32_t nRoomId, OUT ROOM& room) {
    auto && it = m_mapRoomData.find(nRoomId);
    if (it == m_mapRoomData.end()) {
        UWL_ERR("can not find roomid = %d room data", nRoomId);
        //assert(false);
        return false;
    }

    room = it->second.room;
    return true;
}
uint32_t CRobotMgr::GetRoomDataLastTime(int32_t nRoomId) {
    auto && it = m_mapRoomData.find(nRoomId);
    if (it == m_mapRoomData.end())
        return 0;
    return it->second.nLastGetTime;
}
void	CRobotMgr::SetRoomData(int32_t nRoomId, const LPROOM& pRoom) {
    m_mapRoomData[nRoomId] = HallRoomData{*pRoom, time(nullptr)};
}



RobotPtr CRobotMgr::GetRobotByToken(const EConnType& type, const TokenID& id) {
    std::lock_guard<std::mutex> lock(robot_map_mutex_);
    auto it = std::find_if(robot_map_.begin(), robot_map_.end(), [&] (const std::pair<UserID, RobotPtr>& it) {
        switch (type) {
            case ECT_ROOM:
                return it.second->RoomToken() == id;
                break;
            case ECT_GAME:
                return it.second->GameToken() == id;
                break;
            default:
                return false;
        }
    });

    return it != robot_map_.end() ? it->second : nullptr;
}

//try to enter room amount of robots
int32_t CRobotMgr::ApplyRobotForRoom(int32_t nGameId, int32_t nRoomId, int32_t nMaxCount) {
    if (!hall_connection_) {
        UWL_ERR("ApplyRobotForRoom m_ConnHall is nil nGameId = %d, nRoomId = %d, nMaxCount = %d", nGameId, nRoomId, nMaxCount);
        assert(false);
        return 0;
    }


    if (!hall_connection_->IsConnected()) {
        UWL_ERR("ApplyRobotForRoom m_ConnHall not connected nGameId = %d, nRoomId = %d, nMaxCount = %d", nGameId, nRoomId, nMaxCount);
        assert(false);
        return 0;
    }


    int32_t n = 0, nErr = 0;

    // 1 from AcntSett to logon
    for (; n < nMaxCount; n++) {
        //UWL_INF("ApplyRobotForRoom 1 CALL RobotLogonHall");
        auto&& tRet = RobotLogonHall();
        if (!TUPLE_ELE(tRet, 0)) {
            UWL_ERR("ApplyRobotForRoom m_ConnHall not connected nGameId = %d, nRoomId = %d, nMaxCount = %d", nGameId, nRoomId, nMaxCount);
            UWL_ERR("ApplyRobotForRoom1 logon Err:%s", TUPLE_ELE_C(tRet, 1));
            assert(false);
            nErr++;
            continue;
        }
    }

    // 2 req RoomData from hall
    auto&& tRet = SendGetRoomData(nRoomId);
    if (!TUPLE_ELE(tRet, 0)) {
        UWL_ERR("ApplyRobotForRoom m_ConnHall not connected nGameId = %d, nRoomId = %d, nMaxCount = %d", nGameId, nRoomId, nMaxCount);
        UWL_WRN("ApplyRobotForRoom1 SendGetRoomData Err:%s", TUPLE_ELE_C(tRet, 1));
        assert(false);
        return 0;
    }
    // 3 from UIdRobot to enter
    n = 0;
    {
        std::lock_guard<std::mutex> lock(robot_map_mutex_);
        std::unordered_map<UserID, RobotPtr> filterRobotMap;
        //filter user 
        for (auto&& it = robot_map_.begin(); it != robot_map_.end() && n < nMaxCount; it++) {
            if (!it->second->IsLogon()) {
                continue;
            }
            if (it->second->GetPlayerRoomStatus() == ROOM_PLAYER_STATUS_PLAYING) {
                continue;
            }

            if (it->second->GetPlayerRoomStatus() == ROOM_PLAYER_STATUS_WAITING) {
                continue;
            }

            if (it->second->GetRoomID() != 0) {
                continue;
            }

            if (it->second->IsGaming()) {
                continue;
            }

            filterRobotMap[it->second->GetUserID()] = it->second;
        }

        UWL_INF("[interval] filterRobotMap size = %d, timestamp = %I32u", filterRobotMap.size(), time(nullptr));
        if (filterRobotMap.size() == 0) {
            UWL_ERR("[NOTE][ROBOT RANDOM NOT FOUND]");
            return 0;
        }
        //

        auto it = filterRobotMap.begin();
        auto randnum = rand();
        auto mapSize = filterRobotMap.size();
        auto num = randnum % mapSize;
        std::advance(it, num);
        ROOM theRoom = {};
        GetRoomData(nRoomId, theRoom);
        if (theRoom.nRoomID == 0) {
            UWL_ERR("account = %d, theRoom.nRoomID = 0 ???", theRoom.nRoomID);
            //assert(false);
            return 0;
        }

        tRet = it->second->SendEnterRoom(theRoom, m_thrdRoomNotify.ThreadId());
        if (!TUPLE_ELE(tRet, 0)) {
            SetLogon(it->first, false);
            auto retStr = TUPLE_ELE_C(tRet, 1);
            UWL_WRN("ApplyRobotForRoom1 Account:%d enterRoom[%d] Err:%s", it->second->GetUserID(), nRoomId, TUPLE_ELE_C(tRet, 1));
            if (strcmp(retStr, "GR_DEPOSIT_NOTENOUGH") != 0
                || strcmp(retStr, "GR_DEPOSIT_OVERFLOW") != 0
                || strcmp(retStr, "GR_DEPOSIT_NOTENOUGH_EX") != 0
                ) {
                if (strcmp(retStr, "ROOM_NEED_DXXW") == 0) {
                    UWL_WRN("[interval] ApplyRobotForRoom1 ROOM_NEED_DXXW Account:%d enterRoom[%d] Err:%s, time = %I32u", it->second->GetUserID(), nRoomId, TUPLE_ELE_C(tRet, 1), time(nullptr));
                    return 0;
                    //@zhuhangmin 如何让机器人离开上个房间
                } else if (strcmp(retStr, "GR_DEPOSIT_NOTENOUGH") == 0) {
                    //@zhuhangmin robot need gain deposit later
                    UWL_WRN("[interval] ApplyRobotForRoom1 GR_DEPOSIT_NOTENOUGH Account:%d enterRoom[%d] Err:%s, time = %I32u", it->second->GetUserID(), nRoomId, TUPLE_ELE_C(tRet, 1), time(nullptr));
                    return 0;

                } else if (strcmp(retStr, "ROOM_SOCKET_ERROR") == 0) {
                    UWL_WRN("[interval] ApplyRobotForRoom1 ROOM_SOCKET_ERROR Account:%d enterRoom[%d] Err:%s, time = %I32u", it->second->GetUserID(), nRoomId, TUPLE_ELE_C(tRet, 1), time(nullptr));
                    OnCliDisconnRoomWithLock(it->second, 0, nullptr, 0);
                    //assert(false);
                    return 0;
                } else {
                    UWL_WRN("[interval] ApplyRobotForRoom1 UNKONW ERR Account:%d enterRoom[%d] Err:%s, time = %I32u", it->second->GetUserID(), nRoomId, TUPLE_ELE_C(tRet, 1), time(nullptr));
                    OnCliDisconnRoomWithLock(it->second, 0, nullptr, 0);
                    /*assert(false);*/
                    return 0;
                }
            }
            n++;
            nErr++; /*continue;*/
        }

        n++;

        UWL_INF("[interval] ApplyRobotForRoom1 enter room OK successfully Room[%d] Account:[%d], time = %I32u", nRoomId, it->second->GetUserID(), time(nullptr));


        //@zhuhangmin 触发自动匹配
        NTF_GET_NEWTABLE lpNewTableInfo;
        tRet = it->second->SendGetNewTable(theRoom, m_thrdRoomNotify.ThreadId(), lpNewTableInfo);
        if (!TUPLE_ELE(tRet, 0)) {
            UWL_ERR("[interval] ApplyRobotForRoom1 Account:%d Room[%d] askNewTable Err:%s time = %I32u", it->second->GetUserID(), nRoomId, TUPLE_ELE_C(tRet, 1), time(nullptr));
            OnCliDisconnRoomWithLock(it->second, 0, nullptr, 0);
            return 0;
        } else {
            if (!(it->second->IsGaming())) // 如果不在游戏服务器 则加入等待进入游戏服务器的列表
            {
                OnRoomRobotEnter(it->second, lpNewTableInfo.pp.nTableNO, lpNewTableInfo.pp.nChairNO, "");
            }
        }

        SetRoomID(it->second->GetUserID(), nRoomId);
    }
    return n;
}

//try to enter room amount of sepcific accounts robot
int32_t CRobotMgr::ApplyRobotForRoom(int32_t nGameId, int32_t nRoomId, TInt32Vec accounts) {
    if (!hall_connection_) {
        UWL_ERR("ApplyRobotForRoom2 m_ConnHall is nil nGameId = %d, nRoomId = %d", nGameId, nRoomId);
        assert(false);
        return 0;
    }

    if (!hall_connection_->IsConnected()) {
        UWL_ERR("ApplyRobotForRoom2 m_ConnHall not connected nGameId = %d, nRoomId = %d", nGameId, nRoomId);
        assert(false);
        return 0;
    }

    int32_t n = 0, nErr = 0;
    // 1 from AcntSett to logon
    for (auto&& it = accounts.begin(); it != accounts.end(); it++) {
        //UWL_INF("ApplyRobotForRoom 2 CALL RobotLogonHall");
        auto&& tRet = RobotLogonHall(*it);
        if (!TUPLE_ELE(tRet, 0)) {
            nErr++;
            UWL_WRN("ApplyRobotForRoom2 Account:%d logon Err:%s", *it, TUPLE_ELE_C(tRet, 1));
            UWL_ERR("ApplyRobotForRoom2 m_ConnHall not connected nGameId = %d, nRoomId = %d", nGameId, nRoomId);
            assert(false);
            continue;
        }
    }
    // 2 req RoomData from hall
    auto&& tRet = SendGetRoomData(nRoomId);
    if (!TUPLE_ELE(tRet, 0)) {
        UWL_WRN("ApplyRobotForRoom2 SendGetRoomData Err:%s", TUPLE_ELE_C(tRet, 1));
        UWL_ERR("ApplyRobotForRoom2 m_ConnHall not connected nGameId = %d, nRoomId = %d", nGameId, nRoomId);
        assert(false);
        return 0;
    }
    // 3 from AntRobot to enter
    {
        std::lock_guard<std::mutex> lock(robot_map_mutex_);
        for (auto&& it = accounts.begin(); it != accounts.end(); it++) {
            auto&& it_ = robot_map_.find(*it); // 已经登陆大厅的机器人
            if (it_ == robot_map_.end()) continue;

            if (!it_->second->IsLogon()) continue;

            if (it_->second->GetRoomID() != 0)	continue;

            ROOM theRoom = {};
            GetRoomData(nRoomId, theRoom);

            tRet = it_->second->SendEnterRoom(theRoom, m_thrdRoomNotify.ThreadId());
            if (!TUPLE_ELE(tRet, 0)) {
                SetLogon(it_->first, false);
                UWL_WRN("ApplyRobotForRoom2 Account:%d enterRoom[%d] Err:%s", it_->second->GetUserID(), nRoomId, TUPLE_ELE_C(tRet, 1));
                auto retStr = TUPLE_ELE_C(tRet, 1);
                if (strcmp(retStr, "GR_DEPOSIT_NOTENOUGH") != 0
                    || strcmp(retStr, "GR_DEPOSIT_OVERFLOW") != 0
                    || strcmp(retStr, "GR_DEPOSIT_NOTENOUGH_EX") != 0
                    ) {
                    if (strcmp(retStr, "ROOM_NEED_DXXW") == 0) {
                        //@zhuhangmin 如何让机器人离开上个房间
                        UWL_ERR("ApplyRobotForRoom2 ROOM_NEED_DXXW nGameId = %d, nRoomId = %d", nGameId, nRoomId);
                    } else if (strcmp(retStr, "GR_DEPOSIT_NOTENOUGH") == 0) {
                        //@zhuhangmin robot need gain deposit later
                        UWL_ERR("ApplyRobotForRoom2 GR_DEPOSIT_NOTENOUGH nGameId = %d, nRoomId = %d", nGameId, nRoomId);
                    } else if (strcmp(retStr, "ROOM_REQUEST_TIEM_OUT") == 0) {
                        //@zhuhangmin
                        OnCliDisconnRoomWithLock(it_->second, 0, nullptr, 0);
                        UWL_ERR("ApplyRobotForRoom2 ROOM_REQUEST_TIEM_OUT nGameId = %d, nRoomId = %d", nGameId, nRoomId);
                    } else if (strcmp(retStr, "ROOM_SOCKET_ERROR") == 0) {
                        UWL_ERR("ApplyRobotForRoom2 ROOM_SOCKET_ERROR nGameId = %d, nRoomId = %d", nGameId, nRoomId);
                        //assert(false);
                        OnCliDisconnRoomWithLock(it_->second, 0, nullptr, 0);
                    } else {
                        UWL_ERR("ApplyRobotForRoom2 UNKNOW ERROR  nGameId = %d, nRoomId = %d", nGameId, nRoomId);
                        assert(false);
                    }
                }
            }

            n++;
            SetRoomID(it_->second->GetUserID(), nRoomId);


            //@zhuhangmin 触发自动匹配
            NTF_GET_NEWTABLE lpNewTableInfo;
            tRet = it_->second->SendGetNewTable(theRoom, m_thrdRoomNotify.ThreadId(), lpNewTableInfo);
            if (!TUPLE_ELE(tRet, 0)) {
                UWL_ERR("ApplyRobotForRoom1 Account:%d Room[%d] askNewTable Err:%s", it_->second->GetUserID(), nRoomId, TUPLE_ELE_C(tRet, 1));
                assert(false);
            } else {
                if (!(it_->second->IsGaming())) {
                    OnRoomRobotEnter(it_->second, lpNewTableInfo.pp.nTableNO, lpNewTableInfo.pp.nChairNO, "");
                }
            }
        }
    }
    return n;
}

void	CRobotMgr::ThreadRunHallNotify() {
    UWL_INF(_T("HallNotify thread started. id = %d"), GetCurrentThreadId());

    MSG msg = {};
    while (GetMessage(&msg, 0, 0, 0)) {
        if (UM_DATA_RECEIVED == msg.message) {

            LPCONTEXT_HEAD	pContext = (LPCONTEXT_HEAD) (msg.wParam);
            LPREQUEST		pRequest = (LPREQUEST) (msg.lParam);

            TokenID		nTokenID = pContext->lTokenID;
            RequestID		nReqstID = pRequest->head.nRequest;

            OnHallNotify(nReqstID, pRequest->pDataPtr, pRequest->nDataLen);

            SAFE_DELETE(pContext);
            UwlClearRequest(pRequest);
            SAFE_DELETE(pRequest);
        } else {
            DispatchMessage(&msg);
        }
    }
    UWL_INF(_T("HallNotify thread exiting. id = %d"), GetCurrentThreadId());
    return;
}
void	CRobotMgr::ThreadRunRoomNotify() {
    UWL_INF(_T("RoomNotify thread started. id = %d"), GetCurrentThreadId());

    MSG msg = {};
    while (GetMessage(&msg, 0, 0, 0)) {
        if (UM_DATA_RECEIVED == msg.message) {

            LPCONTEXT_HEAD	pContext = (LPCONTEXT_HEAD) (msg.wParam);
            LPREQUEST		pRequest = (LPREQUEST) (msg.lParam);

            TokenID		nTokenID = pContext->lTokenID;
            RequestID		nReqstID = pRequest->head.nRequest;

            auto client = GetRobotByToken(ECT_ROOM, nTokenID);
            if (client == nullptr) {
                UWL_WRN(_T("RoomNotify client not found. nTokenID = %d"), nTokenID);
                continue;
            }

            OnRoomNotify(client, nReqstID, pRequest->pDataPtr, pRequest->nDataLen);

            SAFE_DELETE(pContext);
            UwlClearRequest(pRequest);
            SAFE_DELETE(pRequest);
        } else {
            DispatchMessage(&msg);
        }
    }
    UWL_INF(_T("RoomNotify thread exiting. id = %d"), GetCurrentThreadId());
    return;
}
void	CRobotMgr::ThreadRunGameNotify() {
    UWL_INF(_T("GameNotify thread started. id = %d"), GetCurrentThreadId());

    MSG msg = {};
    while (GetMessage(&msg, 0, 0, 0)) {
        if (UM_DATA_RECEIVED == msg.message) {

            LPCONTEXT_HEAD	pContext = (LPCONTEXT_HEAD) (msg.wParam);
            LPREQUEST		pRequest = (LPREQUEST) (msg.lParam);

            TokenID		nTokenID = pContext->lTokenID;
            RequestID		nReqstID = pRequest->head.nRequest;

            auto client = GetRobotByToken(ECT_GAME, nTokenID);
            if (client == nullptr) {
                UWL_WRN(_T("GameNotify client not found. nTokenID = %d reqId:%d"), nTokenID, nReqstID);
                continue;
            }

            OnGameNotify(client, nReqstID, pRequest->pDataPtr, pRequest->nDataLen);

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

void	CRobotMgr::ThreadRunGameInfoNotify() {
    UWL_INF(_T("GameNotify thread started. id = %d"), GetCurrentThreadId());

    MSG msg = {};
    while (GetMessage(&msg, 0, 0, 0)) {
        if (UM_DATA_RECEIVED == msg.message) {

            LPCONTEXT_HEAD	pContext = (LPCONTEXT_HEAD) (msg.wParam);
            LPREQUEST		pRequest = (LPREQUEST) (msg.lParam);

            TokenID		nTokenID = pContext->lTokenID;
            RequestID		nReqstID = pRequest->head.nRequest;

            auto client = GetRobotByToken(ECT_GAME, nTokenID);
            if (client == nullptr) {
                UWL_WRN(_T("GameNotify client not found. nTokenID = %d reqId:%d"), nTokenID, nReqstID);
                continue;
            }

            OnGameNotify(client, nReqstID, pRequest->pDataPtr, pRequest->nDataLen);

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
void    CRobotMgr::ThreadRunEnterGame() {
    UWL_INF(_T("EnterGame thread started. id = %d"), GetCurrentThreadId());

    while (TRUE) {
        DWORD dwRet = WaitForSingleObject(g_hExitServer, MIN_TIMER_INTERVAL);
        if (WAIT_OBJECT_0 == dwRet) { // exit event
            break;
        }
        if (WAIT_TIMEOUT == dwRet) { // timeout
            //UWL_DBG("[interval] **** ThreadRunEnterGame beg interval = %d, time = %I32u", MIN_TIMER_INTERVAL, time(nullptr));
            //OnThrndDelyEnterGame(time(nullptr));
        }
    }
    UWL_INF(_T("EnterGame thread exiting. id = %d"), GetCurrentThreadId());
    return;
}

void CRobotMgr::OnHallNotify(RequestID nReqId, void* pDataPtr, int32_t nSize) {
    switch (nReqId) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
            OnCliDisconnHallWithLock(nReqId, pDataPtr, nSize);
            break;

        case GR_GET_ROOMUSERS_OK:
            OnHallRoomUsersOK(nReqId, pDataPtr);
            return;
    }
}
void CRobotMgr::OnRoomNotify(RobotPtr client, RequestID nReqId, void* pDataPtr, int32_t nSize) {
    if (nReqId == GR_PLAYER_ENTERED ||
        nReqId == GR_PLAYER_ENTERED ||
        nReqId == GR_PLAYER_SEATED ||
        nReqId == GR_PLAYER_STARTED ||
        nReqId == GR_PLAYER_NEWTABLE ||
        nReqId == GR_PLAYER_RESULT ||
        nReqId == GR_PLAYER_LEAVETABLE ||
        nReqId == GR_PLAYER_GAMEBOUTEND ||
        nReqId == GR_PLAYER_GAMESTARTUP ||
        nReqId == GR_PLAYER_LEFT ||
        nReqId == GR_SOLORANDOM_PLAYING

        ) {

    } else {
        UWL_INF("OnRoomNotify client[%d] reqId:%d", client->GetUserID(), nReqId);
    }

    switch (nReqId) {
        case GR_PLAYER_ENTERED:
        case GR_PLAYER_SEATED:
        case GR_PLAYER_STARTED:
        case GR_PLAYER_NEWTABLE:
        case GR_PLAYER_RESULT:
        case GR_PLAYER_LEAVETABLE:
        case GR_PLAYER_GAMEBOUTEND:
        case GR_PLAYER_GAMESTARTUP:
            return;
    }

    bool bEnterGame = false;
    std::string sEnterWay = "";
    int32_t nTableNo = 0, nChairNo = 0;

    switch (nReqId) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
            OnCliDisconnRoomWithLock(client, nReqId, pDataPtr, nSize);
            break;
        case GR_NEED_ENTERGAME: // solo房开始并进入游戏
        {
            LPPLAYER_POSITION  pp = (LPPLAYER_POSITION) pDataPtr;
            bEnterGame = true;
            sEnterWay = "GR_NEED_ENTERGAME";
            nTableNo = pp->nTableNO; nChairNo = pp->nChairNO;
        }
        break;
        case GR_PLAYER_PLAYING: // 普通房人满开始游戏
        {
            LPNTF_GET_STARTED  np = (LPNTF_GET_STARTED) pDataPtr;
            bEnterGame = true;
            sEnterWay = "GR_PLAYER_PLAYING";
            nTableNo = np->pp.nTableNO; nChairNo = np->pp.nChairNO;
        }
        break;
        //case GR_SOLORANDOM_PLAYING:// 随机solo房配桌ok开始游戏
        //	break;
        case GR_RANDOM_PLAYING: // 随机非solo房配桌ok开始游戏
        {
            LPRANDOM_PLAYING  rp = (LPRANDOM_PLAYING) pDataPtr;
            bEnterGame = true;
            sEnterWay = "GR_PLAYER_PLAYING";
            nTableNo = rp->pp.nTableNO; nChairNo = rp->pp.nChairNO;
        }
        break;

        case GR_SOLOTABLE_CLOSED:
        {
            client->SetPlayerRoomStatus(PLAYER_STATUS_OFFLINE);
        }
        break;
    }
    if (bEnterGame && !client->IsGaming()) {
        OnRoomRobotEnter(client, nTableNo, nChairNo, sEnterWay);
    }
}
void CRobotMgr::OnGameNotify(RobotPtr client, RequestID nReqId, void* pDataPtr, int32_t nSize) {
    if (nReqId == 229500 ||
        (nReqId >= 402000 && nReqId <= 4030000) ||
        nReqId == 211080 ||
        nReqId == 211208 ||
        nReqId == 211010 ||
        nReqId == 211011 ||
        nReqId == 211200 ||
        nReqId == 211207
        ) {

    } else {
        UWL_INF("OnGameNotify client[%d] reqId:%d", client->GetUserID(), nReqId);
    }
    switch (nReqId) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
            OnCliDisconnGameWithLock(client, nReqId, pDataPtr, nSize);
            break;
        case 211010: //GR_PLAYER_ABORT
            break;

        case 211028://  GR_START_SOLOTABLE
        {
            client->SetPlayerRoomStatus(ROOM_PLAYER_STATUS_PLAYING);
        }
        break;
    }
}
void	CRobotMgr::OnHallRoomUsersOK(RequestID nReqId, void* pDataPtr) {
    std::lock_guard<std::mutex> lock(robot_map_mutex_);
    ITEM_COUNT* pItemCount = (ITEM_COUNT*) pDataPtr;
    ITEM_USERS* pItemUsers = (ITEM_USERS*) ((PBYTE) pDataPtr + sizeof(ITEM_COUNT));
    for (int32_t i = 0; i < pItemCount->nCount; i++, pItemUsers++) {
        for (auto&& it_ = room_cur_users_.begin(); it_ != room_cur_users_.end(); it_++) {
            if (it_->first == pItemUsers->nItemID)
                it_->second = pItemUsers->nUsers;
        }
    }
}
void CRobotMgr::OnRoomRobotEnter(RobotPtr client, int32_t nTableNo, int32_t nChairNo, std::string sEnterWay) {
    client->m_nToTable = nTableNo;
    client->m_nToChair = nChairNo;
    client->m_sEnterWay = sEnterWay;
    //UWL_INF("AddWaitEnter 已进入房间，等待进入游戏的机器人 %s: client[%d] room[%d].", sEnterWay.c_str(), client->GetUserID(), client->RoomId());
}
void CRobotMgr::OnCliDisconnHallWithLock(RequestID nReqId, void* pDataPtr, int32_t nSize) {
    UWL_ERR(_T("与大厅服务断开连接"));
    assert(false);
    hall_connection_->DestroyEx();
    {
        for (auto&& it = robot_map_.begin(); it != robot_map_.end(); it++) {
            it->second->SetLogon(false);
        }
    }
}
void CRobotMgr::OnCliDisconnRoomWithLock(RobotPtr client, RequestID nReqId, void* pDataPtr, int32_t nSize) {
    UWL_WRN("userid = %d disconnect room %d", client->GetUserID(), client->GetRoomID());
    SetRoomID(client->GetUserID(), 0);
    client->OnDisconnRoom();

}
void CRobotMgr::OnCliDisconnGameWithLock(RobotPtr client, RequestID nReqId, void* pDataPtr, int32_t nSize) {
    UWL_WRN("userid = %d disconnect game ", client->GetUserID());
    client->OnDisconnGame();
}

TTueRet CRobotMgr::RobotLogonHall(const int32_t& account) {

    AccountSettingMap::iterator itRobot = account_setting_map_.end();

    //配置账号是否存在
    // 指定账号 否则随机
    if (account != 0) {
        auto&& it = account_setting_map_.find(account); // robot setting
        if (it == account_setting_map_.end())
            return std::make_tuple(false, "使用无效的机器人账号");

        if (IsLogon(account))
            return std::make_tuple(true, "不可重复登录");

        itRobot = it;
    } else {
        //random
        for (int i = 0; i < 10; i++) {
            auto it = account_setting_map_.begin();
            auto randnum = rand();
            auto mapSize = account_setting_map_.size();
            auto num = randnum % mapSize;
            std::advance(it, num);
            if (IsLogon(it->first)) continue;
            itRobot = it; break;
        }

    }

    if (itRobot == account_setting_map_.end()) {

        if (account == 0) {
            //UWL_DBG("没有剩余机器人账号数据可用于登陆大厅");
        } else {
            UWL_ERR("没有找到机器人账号数据 account = %d", account);
            assert(false);
        }
        return std::make_tuple(true, "没有找到机器人账号数据可用于登陆大厅");
    }

    //账号对应client是否已经生成, 是否已经登入大厅
    auto client = GetRobotClient(itRobot->first);
    if (!client)
        client = std::make_shared<Robot>(itRobot->second);

    if (client->IsLogon())
        return std::make_tuple(false, "已经登录");

    LOGON_USER_V2  logonUser = {};
    logonUser.nUserID = client->GetUserID();
    xyGetHardID(logonUser.szHardID);  // 硬件ID
    xyGetVolumeID(logonUser.szVolumeID);
    xyGetMachineID(logonUser.szMachineID);
    UwlGetSystemVersion(logonUser.dwSysVer);
    logonUser.nAgentGroupID = ROBOT_AGENT_GROUP_ID; // 使用888作为组号
    logonUser.dwLogonFlags |= (FLAG_LOGON_INTER | FLAG_LOGON_ROBOT_TOOL);
    logonUser.nLogonSvrID = 0;
    logonUser.nHallBuildNO = 20160414;
    logonUser.nHallNetDelay = 1;
    logonUser.nHallRunCount = 1;
    strcpy_s(logonUser.szPassword, client->Password().c_str());

    RequestID nResponse;
    //LPVOID	 pRetData = NULL;
    std::shared_ptr<void> pRetData;
    uint32_t nDataLen = sizeof(logonUser);
    auto it = SendHallRequest(GR_LOGON_USER_V2, nDataLen, &logonUser, nResponse, pRetData);
    if (!TUPLE_ELE(it, 0)) {
        UWL_ERR("ACCOUNT = %d GR_LOGON_USER_V2 FAIL", client->GetUserID());
        return it;
    }

    if (!(nResponse == GR_LOGON_SUCCEEDED || nResponse == GR_LOGON_SUCCEEDED_V2)) {
        UWL_ERR("ACCOUNT = %d GR_LOGON_USER_V2 FAIL", client->GetUserID());
        return std::make_tuple(false, std::to_string(nResponse));
    }

    client->SetLogon(true);
    client->SetLogonData((LPLOGON_SUCCEED_V2) pRetData.get());
    SetRobotClient(client);

    UWL_INF("account:%d userid:%d logon hall ok.", client->GetUserID(), client->GetUserID());
    return std::make_tuple(true, ERR_OPERATE_SUCESS);
}

TTueRet CRobotMgr::SendGetRoomData(const int32_t nRoomId) {
    if (time(nullptr) - GetRoomDataLastTime(nRoomId) < 30) {
        //UWL_INF("NO NEED TO UPDATE GetRoomDataLastTime nRoomId = %d, interval = %I32u", nRoomId, (time(nullptr) - GetRoomDataLastTime(nRoomId)));
        return std::make_tuple(true, ERR_NOT_NEED_OPERATE);
    }


    GET_ROOM  gr = {};
    gr.nGameID = g_gameID;
    gr.nRoomID = nRoomId;
    gr.nAgentGroupID = ROBOT_AGENT_GROUP_ID;
    gr.dwFlags |= FLAG_GETROOMS_INCLUDE_HIDE;
    gr.dwFlags |= FLAG_GETROOMS_INCLUDE_ONLINES;

    RequestID nResponse;
    //LPVOID	 pRetData = NULL;
    std::shared_ptr<void> pRetData;
    uint32_t nDataLen = sizeof(GET_ROOM);
    auto _it = SendHallRequest(GR_GET_ROOM, nDataLen, &gr, nResponse, pRetData);
    if (!TUPLE_ELE(_it, 0)) {
        UWL_ERR("SendHallRequest GR_GET_ROOM fail nRoomId = %d, nResponse = %d", nRoomId, nResponse);
        assert(false);
        return _it;
    }


    if (nResponse != UR_FETCH_SUCCEEDED) {
        UWL_ERR("SendHallRequest GR_GET_ROOM fail nRoomId = %d, nResponse = %d", nRoomId, nResponse);
        assert(false);
        return std::make_tuple(false, std::to_string(nResponse));
    }
    SetRoomData(nRoomId, (LPROOM) pRetData.get());
    return std::make_tuple(true, ERR_OPERATE_SUCESS);
}
TTueRet CRobotMgr::RobotGainDeposit(RobotPtr client) {
    static TCHAR szHttpUrl[128];
    static int nResult = GetPrivateProfileString(_T("RobotReward"), _T("HttpUrl_G"), _T("null"), szHttpUrl, sizeof(szHttpUrl), g_szIniFile);
    if (0 == strcmp(szHttpUrl, "null"))
        return std::make_tuple(false, "RobotReward HttpUrl_G ini read faild");

    TCHAR szField[128] = {};
    sprintf_s(szField, "ActiveID_Game%d", g_gameID);
    TCHAR szValue[128] = {};
    GetPrivateProfileString(_T("RobotReward"), szField, _T("null"), szValue, sizeof(szValue), g_szIniFile);
    if (0 == strcmp(szValue, "null"))
        return std::make_tuple(false, "RobotGainDeposit ActiveId ini read faild");

    Json::Value root;
    Json::FastWriter fast_writer;
    root["GameId"] = Json::Value(g_gameID);
    root["ActId"] = Json::Value(szValue);
    root["UserId"] = Json::Value(client->GetUserID());
    root["UserNickName"] = Json::Value("Robot");
    root["Num"] = Json::Value(client->m_nGainAmount);
    root["Operation"] = Json::Value(1);
    root["Device"] = Json::Value(xyConvertIntToStr(g_gameID) + "+" + xyConvertIntToStr(client->GetUserID()));
    root["Time"] = Json::Value((LONGLONG) time(NULL) * 1000);
    root["Sign"] = Json::Value(MD5String((root["ActId"].asString() + "|" + root["UserId"].asString() + "|" + root["Time"].asString() + "|" + ROBOT_APPLY_DEPOSIT_KEY).c_str()));

    CString strParam = fast_writer.write(root).c_str();
    CString strResult = ExecHttpRequestPost(szHttpUrl, strParam);
    Json::Reader reader;
    Json::Value _root;
    if (!reader.parse((LPCTSTR) strResult, _root))
        return std::make_tuple(false, "RobotGainDeposit Http result parse faild");
    if (_root["Code"].asInt() != 0) {
        UWL_ERR("m_Account = %d gain deposit fail, code = %d, strResult = %s", client->GetUserID(), _root["Code"].asInt(), strResult);
        //assert(false);
        return std::make_tuple(false, "RobotGainDeposit Http result Status false");
    }

    return std::make_tuple(true, ERR_OPERATE_SUCESS);
}

TTueRet CRobotMgr::RobotBackDeposit(RobotPtr client) {
    static TCHAR szHttpUrl[128];
    static int nResult = GetPrivateProfileString(_T("RobotReward"), _T("HttpUrl_C"), _T("null"), szHttpUrl, sizeof(szHttpUrl), g_szIniFile);
    if (0 == strcmp(szHttpUrl, "null"))
        return std::make_tuple(false, "RobotReward HttpUrl_C ini read faild");

    TCHAR szField[128] = {};
    sprintf_s(szField, "ActiveID_Game%d", g_gameID);
    TCHAR szValue[128] = {};
    GetPrivateProfileString(_T("RobotReward"), szField, _T("null"), szValue, sizeof(szValue), g_szIniFile);
    if (0 == strcmp(szValue, "null"))
        return std::make_tuple(false, "RobotBackDeposit ActiveId ini read faild");

    Json::Value root;
    Json::FastWriter fast_writer;
    root["GameId"] = Json::Value(g_gameID);
    root["ActId"] = Json::Value(szValue);
    root["UserId"] = Json::Value(client->GetUserID());
    root["UserNickName"] = Json::Value("Robot");
    root["Num"] = Json::Value(client->m_nBackAmount);
    root["Operation"] = Json::Value(2);
    root["Device"] = Json::Value(xyConvertIntToStr(g_gameID) + "+" + xyConvertIntToStr(client->GetUserID()));
    root["Time"] = Json::Value((LONGLONG) time(NULL) * 1000);
    root["Sign"] = Json::Value(MD5String((root["ActId"].asString() + "|" + root["UserId"].asString() + "|" + root["Time"].asString() + "|" + ROBOT_APPLY_DEPOSIT_KEY).c_str()));

    CString strParam = fast_writer.write(root).c_str();
    CString strResult = ExecHttpRequestPost(szHttpUrl, strParam);
    Json::Reader reader;
    Json::Value _root;
    if (!reader.parse((LPCTSTR) strResult, _root))
        return std::make_tuple(false, "RobotBackDeposit Http result parse faild");
    if (!_root["Status"].asBool())
        return std::make_tuple(false, "RobotBackDeposit Http result Status false");
    return std::make_tuple(true, ERR_OPERATE_SUCESS);
}

bool CRobotMgr::IsLogon(UserID userid) {
    if (robot_map_.find(userid) != robot_map_.end()) {
        return robot_map_[userid]->IsLogon();
    }
    return false;
}

void CRobotMgr::SetLogon(UserID userid, bool status) {
    if (robot_map_.find(userid) != robot_map_.end()) {
        robot_map_[userid]->SetLogon(status);
    }
}

RobotPtr CRobotMgr::GetRobotClient(UserID userid) {
    if (robot_map_.find(userid) != robot_map_.end()) {
        return robot_map_[userid];
    }
    return nullptr;
}

void CRobotMgr::SetRobotClient(RobotPtr client) {
    robot_map_.insert(std::make_pair(client->GetUserID(), client));
}

bool CRobotMgr::IsInRoom(UserID userid) {
    if (robot_map_.find(userid) != robot_map_.end()) {
        return robot_map_[userid]->IsInRoom();
    }
    return false;
}

void CRobotMgr::SetRoomID(UserID userid, RoomID roomid) {
    if (robot_map_.find(userid) != robot_map_.end()) {
        robot_map_[userid]->SetRoomID(roomid);
    }
}

int CRobotMgr::GetRoomCurrentRobotSize(RoomID roomid) {
    auto count = 0;
    for (auto& kv : robot_map_) {
        if (kv.second->GetRoomID() == roomid) {
            count++;
        }
    }
    return count;
}


void    CRobotMgr::OnServerMainTimer(time_t nCurrTime) {
    OnTimerLogonHall(nCurrTime);
    OnTimerSendHallPluse(nCurrTime);
    OnTimerSendRoomPluse(nCurrTime);
    OnTimerSendGamePluse(nCurrTime);
    OnTimerCtrlRoomActiv(nCurrTime);
    OnTimerUpdateDeposit(nCurrTime);
}
bool	CRobotMgr::OnTimerLogonHall(time_t nCurrTime) {
    if (!hall_connection_) {
        UWL_ERR("SendHallRequest OnTimerReconnectHall nil ERR_CONNECT_NOT_EXIST");
        assert(false);
        return false;
    }


    if (hall_connection_->IsConnected()) return true;

    //@zhuhangmin 10s重连代码，200+机器人 100s 重连 2000多次, 回导致短时间大厅登陆压力过大
#define MAIN_RECONNECT_HALL_GAP_TIME (60) // 60s
    static time_t	sLastReconnectHallGapTime = nCurrTime;
    if (nCurrTime - sLastReconnectHallGapTime >= MAIN_RECONNECT_HALL_GAP_TIME)
        sLastReconnectHallGapTime = nCurrTime;
    else return false;

    if (!InitConnectHall()) {
        UWL_ERR("InitConnectHall");
        assert(false);
        return false;
    }

    int32_t nCount = 0;
    std::vector<int32_t> vecAccounts;
    {
        for (auto&& it = account_setting_map_.begin(); it != account_setting_map_.end(); it++) {
            if (!IsLogon(it->first)) continue;

            if (++nCount >= 5)		 break;

            vecAccounts.push_back(it->second.account);
        }
    }
    for (auto&& it : vecAccounts) {
        //UWL_INF("OnTimerReconnectHall CALL RobotLogonHall");
        auto&& tRet = RobotLogonHall(it);
        if (!TUPLE_ELE(tRet, 0)) {
            assert(false);
            UWL_WRN("OnTimerReconnectHall Account:%d logon Err:%s", it, TUPLE_ELE_C(tRet, 1));
        }
    }
    return true;
}
void    CRobotMgr::OnTimerSendHallPluse(time_t nCurrTime) {
    if (!hall_connection_) {
        UWL_ERR("SendHallRequest OnTimerSendHallPluse nil ERR_CONNECT_NOT_EXIST nReqId");
        assert(false);
        return;
    }


    if (!hall_connection_->IsConnected()) {
        UWL_ERR("SendHallRequest OnTimerSendHallPluse not connect ERR_CONNECT_DISABLE nReqId");
        assert(false);
        return;
    }


#define MAIN_SEND_HALL_PULSE_GAP_TIME (1*60) // 1分钟
    static time_t	sLastSendHPulseGapTime = nCurrTime;
    if (nCurrTime - sLastSendHPulseGapTime >= MAIN_SEND_HALL_PULSE_GAP_TIME)
        sLastSendHPulseGapTime = nCurrTime;
    else return;
    {
        HALLUSER_PULSE hp = {};
        hp.nUserID = 0;
        hp.nAgentGroupID = ROBOT_AGENT_GROUP_ID;

        RequestID nResponse;
        //LPVOID	 pRetData = NULL;
        std::shared_ptr<void> pRetData;
        uint32_t nDataLen = sizeof(HALLUSER_PULSE);
        (void) SendHallRequest(GR_HALLUSER_PULSE, nDataLen, &hp, nResponse, pRetData, false);
    }
}
void    CRobotMgr::OnTimerSendRoomPluse(time_t nCurrTime) {
#define MAIN_SEND_ROOM_PULSE_GAP_TIME (1*10+1) // 1分钟1秒
    static time_t	sLastSendRPulseGapTime = nCurrTime;
    if (nCurrTime - sLastSendRPulseGapTime >= MAIN_SEND_ROOM_PULSE_GAP_TIME)
        sLastSendRPulseGapTime = nCurrTime;
    else return;
    {
        std::lock_guard<std::mutex> lock(robot_map_mutex_);
        for (auto&& it = robot_map_.begin(); it != robot_map_.end(); it++) {
            if (it->second->GetRoomID() == 0)	continue;

            it->second->SendRoomPulse();
        }
    }
}
void    CRobotMgr::OnTimerSendGamePluse(time_t nCurrTime) {
#define MAIN_SEND_GAME_PULSE_GAP_TIME (1*60+2) // 1分钟2秒
    static time_t	sLastSendGPulseGapTime = nCurrTime;
    if (nCurrTime - sLastSendGPulseGapTime >= MAIN_SEND_GAME_PULSE_GAP_TIME)
        sLastSendGPulseGapTime = nCurrTime;
    else return;
    {
        std::lock_guard<std::mutex> lock(robot_map_mutex_);
        for (auto&& it = robot_map_.begin(); it != robot_map_.end(); it++) {
            if (!it->second->IsGaming())	continue;

            it->second->SendGamePulse();
        }
    }
}
void    CRobotMgr::OnTimerCtrlRoomActiv(time_t nCurrTime) {
    if (!hall_connection_) {
        UWL_ERR("SendHallRequest OnTimerCtrlRoomActiv nil ERR_CONNECT_NOT_EXIST");
        assert(false);
        return;
    }

    if (!hall_connection_->IsConnected()) {
        UWL_ERR("SendHallRequest OnTimerCtrlRoomActiv not connect ERR_CONNECT_DISABLE nReqId");
        assert(false);
        return;
    }

    //#define MAIN_ROOM_ACTIV_CHECK_GAP_TIME (8) 
    int MAIN_ROOM_ACTIV_CHECK_GAP_TIME = GetPrivateProfileInt(_T("roominterval"), _T("interval"), 1, g_szIniFile);


    static time_t	sLastRoomActiveCheckGapTime = 0;
    if (nCurrTime - sLastRoomActiveCheckGapTime >= MAIN_ROOM_ACTIV_CHECK_GAP_TIME)
        sLastRoomActiveCheckGapTime = nCurrTime;
    else return;

    UWL_INF("[inteval] enter room beg time = %I32u", time(nullptr));
    GET_ROOMUSERS gr = {};
    gr.nAgentGroupID = ROBOT_AGENT_GROUP_ID;
    std::unordered_map<int32_t, int32_t> roomNeedApply;

    {
        for (auto&& it_2 = room_setting_map_.begin(); it_2 != room_setting_map_.end(); it_2++) {
            if (it_2->second.nCtrlMode == EACM_Hold) {
                gr.nRoomID[gr.nRoomCount] = it_2->second.nRoomId;

                //@zhuhangmin
                /*auto curRoomUsers = it_2->second.nCurrNum;
                auto curRoomID = it_2->second.nRoomId;*/
                //auto curInterval = GetIntervalTime(curRoomUsers);
                //zhuhangmin

                gr.nRoomCount++;

                //zhuhangmin 计算处于各种“状态”的机器人
                int robotInSeating = 0;
                int robotInWaiting = 0;
                int robotInPlaying = 0;
                int unexpectedState = 0;
                //循环某个以在特定房间的用户
                RoomID roomid = it_2->first;
                for (auto&& it_3 = robot_map_.begin(); it_3 != robot_map_.end(); it_3++) {
                    if (it_3->second->GetRoomID() != roomid) continue;
                    int state = it_3->second->GetPlayerRoomStatus();
                    if (state == ROOM_PLAYER_STATUS_PLAYING) {
                        robotInPlaying++;

                    } else if (state == ROOM_PLAYER_STATUS_WAITING) {
                        robotInWaiting++;
                        UWL_DBG("[ROOM STATUS] m_Account = %d is waitting", it_3->second->GetUserID());
                    }
                }
                auto room_robot_size = GetRoomCurrentRobotSize(roomid);
                auto wait_robot_size = 0;
                auto setting_size = it_2->second.nCtrlVal;
                UWL_INF("[ROOM STATUS] room id = %d , already %d,  need %d,  playing %d, waitting %d",
                        it_2->second.nRoomId, room_robot_size, wait_robot_size, robotInPlaying, robotInWaiting);

                if (room_robot_size + wait_robot_size - robotInPlaying < setting_size)
                    if (robotInWaiting < setting_size)
                        roomNeedApply[roomid] = g_gameID;// roomid:gameid
            }
            //if (it_2->second.nCtrlMode == EACM_Scale) {
            //    gr.nRoomID[gr.nRoomCount] = it_2->second.nRoomId;
            //    gr.nRoomCount++;

            //    auto room_robot_num = room_robot_set_[it_2->first].size() + room_want_robot_map_[it_2->first].size();
            //    auto scale_user_num = (size_t) ((it_2->second.nCurrNum - room_robot_num)*it_2->second.nCtrlVal*0.01f);
            //    /*if (it_2->second.nCurrNum > room_robot_num && room_robot_num < scale_user_num)
            //        roomNeedApply[it_2->first] = it_1->first;*/
            //}
        }
    }

    std::unordered_set<RobotPtr>  vecWantClient;
    for (auto&& it : vecWantClient) {
        ApplyRobotForRoom(g_gameID, it->m_nWantRoomId, TInt32Vec{it->GetUserID()});
    }

    for (auto&& it : roomNeedApply) {
        ApplyRobotForRoom(it.second, it.first, 1);
        //ApplyRobotForRoom(it.second, it.first, m_mapAcntSett.size());
    }
    if (gr.nRoomCount > 0) {
        RequestID nResponse;
        //LPVOID	 pRetData = NULL;
        std::shared_ptr<void> pRetData;
        uint32_t nDataLen = sizeof(GET_ROOMUSERS) - sizeof(int)*MAX_QUERY_ROOMS + gr.nRoomCount*sizeof(int);
        (void) SendHallRequest(GR_GET_ROOMUSERS, nDataLen, &gr, nResponse, pRetData, false);
    }
}
void    CRobotMgr::OnTimerUpdateDeposit(time_t nCurrTime) {
    if (!hall_connection_) {
        UWL_ERR("SendHallRequest OnTimerCtrlRoomActiv nil ERR_CONNECT_NOT_EXIST nReqId");
        assert(false);
        return;
    }


    if (!hall_connection_->IsConnected()) {
        UWL_ERR("SendHallRequest OnTimerUpdateDeposit not connect ERR_CONNECT_DISABLE");
        assert(false);
        return;
    }

#define UPDATE_CLIENTNUM_PRE_ONCE	(15)
#define MAIN_ROOM_UPDATE_DEPOSIT_GAP_TIME (15) // 15s
    static time_t	sLastRoomUpdateDepositGapTime = 0;
    if (nCurrTime - sLastRoomUpdateDepositGapTime >= MAIN_ROOM_UPDATE_DEPOSIT_GAP_TIME)
        sLastRoomUpdateDepositGapTime = nCurrTime;
    else return;

    int nCount = 0;
    {
        std::lock_guard<std::mutex> lock(robot_map_mutex_);
        for (auto&& it = robot_map_.begin(); it != robot_map_.end(); it++) {
            if (!it->second->IsLogon())	continue;

            if (it->second->GetRoomID() != 0)	continue;

            if (it->second->IsGaming())		continue;

            if (it->second->m_bGainDeposit) {
                it->second->m_bGainDeposit = false;
                auto&& tRet = RobotGainDeposit(it->second);
                if (!TUPLE_ELE(tRet, 0)) {
                    UWL_WRN("Account:%d GainDeposit Err:%s", it->second->GetUserID(), TUPLE_ELE_C(tRet, 1));
                    continue;
                }
                nCount++;
            }
            if (it->second->m_bBackDeposit) {
                it->second->m_bBackDeposit = false;
                auto&& tRet = RobotBackDeposit(it->second);
                if (!TUPLE_ELE(tRet, 0)) {
                    UWL_WRN("Account:%d BackDeposit Err:%s", it->second->GetUserID(), TUPLE_ELE_C(tRet, 1));
                    continue;
                }
                nCount++;
            }
            if (nCount >= UPDATE_CLIENTNUM_PRE_ONCE)
                break;
        }
    }
}
//void    CRobotMgr::OnThrndDelyEnterGame(time_t nCurrTime) {
//    do {
//        CRobotClient* pRoboter = GetWaitEnter();
//        if (pRoboter == nullptr)	return;
//
//        ROOM theRoom = {};
//        if (!GetRoomData(pRoboter->RoomId(), theRoom)) {
//            UWL_ERR("Room %s: client[%d] room[%d] not found.", pRoboter->m_sEnterWay.c_str(), pRoboter->GetUserID(), pRoboter->RoomId());
//            //assert(false);
//            continue;
//        }
//        stRobotUnit unit;
//        if (!GetRobotSetting(pRoboter->GetUserID(), unit)) {
//            UWL_ERR("Room %s: client[%d] RobotUnit not found.", pRoboter->m_sEnterWay.c_str(), pRoboter->GetUserID());
//            assert(false);
//            continue;
//        }
//
//        auto begTime = GetTickCount();
//        UWL_DBG("[PROFILE] 0 SendEnterGame timestamp = %ld", GetTickCount());
//        auto ret = pRoboter->SendEnterGame(theRoom, m_thrdGameNotify.ThreadId(), unit.nickName, unit.portraitUrl, pRoboter->m_nToTable, pRoboter->m_nToChair);
//        UWL_DBG("[PROFILE] 7 SendEnterGame timestamp = %ld", GetTickCount());
//        if (!TUPLE_ELE(ret, 0)) {
//            UWL_WRN("Room %s: client[%d] enter game faild:%s.", pRoboter->m_sEnterWay.c_str(), pRoboter->GetUserID(), TUPLE_ELE_C(ret, 1));
//            UWL_WRN("Room %s: client[%d] enter game faild try to reset the socket", pRoboter->m_sEnterWay.c_str(), pRoboter->GetUserID());
//            UWL_DBG("[interval] OnThrndDelyEnterGame client[%d] end fail, time = %I32u", pRoboter->GetUserID(), time(nullptr));
//            OnCliDisconnGame(pRoboter, 0, nullptr, 0);
//            continue;
//        }
//        auto endTime = GetTickCount();
//        UWL_DBG("[interval] Room %s: client[%d] Dely Enter Game OK end successfully. time %d ", pRoboter->m_sEnterWay.c_str(), pRoboter->GetUserID(), time(nullptr));
//        UWL_DBG("[interval] [PROFILE] ENTER GAME cost time = %ld", endTime - begTime);
//
//        UWL_INF("[STATUS] client = %d status waitting", pRoboter->GetUserID());
//
//
//    } while (true);
//}