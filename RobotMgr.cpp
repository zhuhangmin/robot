﻿#include "stdafx.h"
#include "RobotMgr.h"
#include "Main.h"
#include "RobotReq.h"

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
    UWL_INF("InitSetting Account Count = %d", m_mapAcntSett.size());

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

    if (!InitGameRoomDatas()) {
        UWL_ERR("InitGameRoomDatas() return false");
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
        mapRooms_[nRoomId] = stActiveCtrl{nRoomId, nCtrlMode, nCtrlValue, 0};
    }

    auto robots = root["robots"];
    size = robots.size();
    for (int i = 0; i < size; ++i) {
        auto robot = robots[i];
        int32_t		nAccount = robot["Account"].asInt();
        std::string sPassword = robot["Password"].asString();
        std::string sNickName = robot["NickName"].asString();
        std::string sPortrait = robot["Portrait"].asString();

        stRobotUnit unit;
        unit.account = nAccount;
        unit.logoned = false;
        unit.password = sPassword;
        unit.nickName = sNickName;
        unit.portraitUrl = sPortrait;
        m_mapAcntSett[nAccount] = unit;
    }
    return closeRet(true);
}
bool	CRobotMgr::InitNotifyThreads() {
    m_thrdHallNotify.Initial(std::thread([this] {this->ThreadRunHallNotify(); }));
    m_thrdRoomNotify.Initial(std::thread([this] {this->ThreadRunRoomNotify(); }));
    m_thrdGameNotify.Initial(std::thread([this] {this->ThreadRunGameNotify(); }));
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
        CAutoLock lock(&m_csConnHall);
        m_ConnHall->InitKey(KEY_HALL, ENCRYPT_AES, 0);

        if (!m_ConnHall->Create(szHallSvrIP, nHallSvrPort, 5, 0, m_thrdHallNotify.ThreadId(), 0, GetHelloData(), GetHelloLength())) {
            UWL_ERR("[ROUTE] ConnectHall Faild! IP:%s Port:%d", szHallSvrIP, nHallSvrPort);
            assert(false);
            return false;
        }
    }
    UWL_INF("ConnectHall[ReConn:%d] OK! IP:%s Port:%d", (int) bReconn, szHallSvrIP, nHallSvrPort);
    return true;
}
bool	CRobotMgr::InitGameRoomDatas() {
    for (auto& itr : mapRooms_) {
        auto&& tRet = SendGetRoomData(itr.first);
        if (!TUPLE_ELE(tRet, 0)) {
            UWL_ERR("SendGetRoomData Faild! nRoomId:%d err:%s", itr.first, TUPLE_ELE_C(tRet, 1));
            assert(false);
        }
    }
    return true;
}

TTueRet CRobotMgr::SendHallRequest(TReqstId nReqId, uint32_t& nDataLen, void *pData, TReqstId &nRespId, void* &pRetData, bool bNeedEcho, uint32_t wait_ms) {
    if (!m_ConnHall) {
        UWL_ERR("SendHallRequest m_CoonHall nil ERR_CONNECT_NOT_EXIST nReqId = %d", nReqId);
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_NOT_EXIST);
    }

    if (!m_ConnHall->IsConnected()) {
        UWL_ERR("SendHallRequest m_CoonHall not connect ERR_CONNECT_DISABLE nReqId = %d", nReqId);
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_DISABLE);
    }

    CONTEXT_HEAD	Context = {};
    REQUEST			Request = {};
    REQUEST			Response = {};
    Context.hSocket = m_ConnHall->GetSocket();
    Context.lSession = 0;
    Context.bNeedEcho = bNeedEcho;
    Request.head.nRepeated = 0;
    Request.head.nRequest = nReqId;
    Request.nDataLen = nDataLen;
    Request.pDataPtr = pData;//////////////

    BOOL bTimeOut = FALSE, bResult = TRUE;
    {
        CAutoLock lock(&m_csConnHall);
        bResult = m_ConnHall->SendRequest(&Context, &Request, &Response, bTimeOut, wait_ms);
    }

    if (!bResult)///if timeout or disconnect 
    {
        UWL_ERR("SendHallRequest m_ConnHall->SendRequest fail bTimeOut = %d, nReqId = %d", bTimeOut, nReqId);
        assert(false);
        return std::make_tuple(false, (bTimeOut ? ERR_SENDREQUEST_TIMEOUT : ERR_OPERATE_FAILED));
    }

    nDataLen = Response.nDataLen;
    nRespId = Response.head.nRequest;
    pRetData = Response.pDataPtr;

    if (nRespId == GR_ERROR_INFOMATION) {
        CHAR info[512] = {};
        sprintf_s(info, "%s", pRetData);
        nDataLen = 0;
        SAFE_DELETE_ARRAY(pRetData);
        UWL_ERR("SendHallRequest m_ConnHall->SendRequest fail nRespId GR_ERROR_INFOMATION nReqId = %d", nReqId);
        assert(false);
        return std::make_tuple(false, info);
    }
    return std::make_tuple(true, ERR_OPERATE_SUCESS);
}

bool	CRobotMgr::GetRobotSetting(int account, OUT stRobotUnit& unit) {
    CAutoLock lock(&m_csRobot);

    if (account == 0)	return false;

    auto it = m_mapAcntSett.find(account);
    if (it == m_mapAcntSett.end()) return false;

    unit = it->second;

    return true;
}
bool	CRobotMgr::GetRoomData(int32_t nRoomId, OUT ROOM& room) {
    CAutoLock lock(&m_csRoomData);
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
    CAutoLock lock(&m_csRoomData);
    auto && it = m_mapRoomData.find(nRoomId);
    if (it == m_mapRoomData.end())
        return 0;
    return it->second.nLastGetTime;
}
void	CRobotMgr::SetRoomData(int32_t nRoomId, const LPROOM& pRoom) {
    CAutoLock lock(&m_csRoomData);
    m_mapRoomData[nRoomId] = stRoomData{*pRoom, time(nullptr)};
}
void	CRobotMgr::AddWaitEnter(CRobotClient* pRoboter) {
    if (pRoboter == nullptr)	return;

    CAutoLock lock(&m_csWaitEnters);
    m_queWaitEnters.push(pRoboter);
}
CRobotClient* CRobotMgr::GetWaitEnter() {
    CAutoLock lock(&m_csWaitEnters);

    if (m_queWaitEnters.empty())	return nullptr;

    auto it = m_queWaitEnters.front();
    if (time(nullptr) == it->m_nNeedEnterG) return nullptr;

    m_queWaitEnters.pop();
    return it;
}

CRobotClient* CRobotMgr::GetRobotClient_ToknId(const EConnType& type, const TTokenId& id) {
    bool bFind = false;
    ToknRobotMap::iterator it;
    CAutoLock lock(&m_csTknRobot);
    switch (type) {
        case ECT_HALL:it = m_mapTknRobot_Hall.find(id); bFind = (it != m_mapTknRobot_Hall.end()); break;
        case ECT_ROOM:it = m_mapTknRobot_Room.find(id); bFind = (it != m_mapTknRobot_Room.end()); break;
        case ECT_GAME:it = m_mapTknRobot_Game.find(id); bFind = (it != m_mapTknRobot_Game.end()); break;
        default: return nullptr;
    }
    return bFind ? it->second : nullptr;
}
CRobotClient* CRobotMgr::GetRobotClient_UserId(const TUserId& id) {
    CAutoLock lock(&m_csRobot);
    auto it = m_mapUIdRobot.find(id);
    return it != m_mapUIdRobot.end() ? it->second : nullptr;
}
CRobotClient* CRobotMgr::GetRobotClient_Accout(const int32_t& account) {
    CAutoLock lock(&m_csRobot);
    auto it = m_mapAntRobot.find(account);
    return it != m_mapAntRobot.end() ? it->second : nullptr;
}
void	CRobotMgr::UpdRobotClientToken(const EConnType& type, CRobotClient* client, bool add) {
    ToknRobotMap* pMap = nullptr;

    if (add && client == nullptr)	return;

    CAutoLock lock(&m_csTknRobot);

    switch (type) {
        case ECT_HALL:return;
        case ECT_ROOM:
            if (add) {
                m_mapTknRobot_Room.insert(std::pair<TTokenId, CRobotClient*>(client->RoomToken(), client));
            } else {
                m_mapTknRobot_Room.erase(client->RoomToken());
            }
            break;
        case ECT_GAME:
            if (add) {
                m_mapTknRobot_Game.insert(std::pair<TTokenId, CRobotClient*>(client->GameToken(), client));
            } else {
                m_mapTknRobot_Game.erase(client->GameToken());
            };
            break;
        default:return;
    }
}

//try to enter room amount of robots
int32_t CRobotMgr::ApplyRobotForRoom(int32_t nGameId, int32_t nRoomId, int32_t nMaxCount) {
    if (!m_ConnHall) {
        UWL_ERR("ApplyRobotForRoom m_ConnHall is nil nGameId = %d, nRoomId = %d, nMaxCount = %d", nGameId, nRoomId, nMaxCount);
        assert(false);
        return 0;
    }


    if (!m_ConnHall->IsConnected()) {
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
        CAutoLock lock(&m_csRobot);
        TUIdRobotMap filterRobotMap;
        //filter user 
        for (auto&& it = m_mapUIdRobot.begin(); it != m_mapUIdRobot.end() && n < nMaxCount; it++) {
            if (it->second->GetPlayerRoomStatus() == ROOM_PLAYER_STATUS_PLAYING) {
                continue;
            }

            if (it->second->GetPlayerRoomStatus() == ROOM_PLAYER_STATUS_WAITING) {
                continue;
            }

            if (it->second->RoomId() != 0) {
                continue;
            }

            if (it->second->IsGaming()) {
                continue;
            }

            if (it->second->m_nWantGameId != 0) {
                continue;
            }
            filterRobotMap[it->second->UserId()] = it->second;
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
            m_mapAcntSett[it->second->Account()].logoned = false;
            auto retStr = TUPLE_ELE_C(tRet, 1);
            UWL_WRN("ApplyRobotForRoom1 Account:%d enterRoom[%d] Err:%s", it->second->Account(), nRoomId, TUPLE_ELE_C(tRet, 1));
            if (strcmp(retStr, "GR_DEPOSIT_NOTENOUGH") != 0
                || strcmp(retStr, "GR_DEPOSIT_OVERFLOW") != 0
                || strcmp(retStr, "GR_DEPOSIT_NOTENOUGH_EX") != 0
                ) {
                if (strcmp(retStr, "ROOM_NEED_DXXW") == 0) {
                    UWL_WRN("[interval] ApplyRobotForRoom1 ROOM_NEED_DXXW Account:%d enterRoom[%d] Err:%s, time = %I32u", it->second->Account(), nRoomId, TUPLE_ELE_C(tRet, 1), time(nullptr));
                    return 0;
                    //@zhuhangmin 如何让机器人离开上个房间
                } else if (strcmp(retStr, "GR_DEPOSIT_NOTENOUGH") == 0) {
                    //@zhuhangmin robot need gain deposit later
                    UWL_WRN("[interval] ApplyRobotForRoom1 GR_DEPOSIT_NOTENOUGH Account:%d enterRoom[%d] Err:%s, time = %I32u", it->second->Account(), nRoomId, TUPLE_ELE_C(tRet, 1), time(nullptr));
                    return 0;

                } else if (strcmp(retStr, "ROOM_SOCKET_ERROR") == 0) {
                    UWL_WRN("[interval] ApplyRobotForRoom1 ROOM_SOCKET_ERROR Account:%d enterRoom[%d] Err:%s, time = %I32u", it->second->Account(), nRoomId, TUPLE_ELE_C(tRet, 1), time(nullptr));
                    OnCliDisconnRoom(it->second, 0, nullptr, 0);
                    //assert(false);
                    return 0;
                } else {
                    UWL_WRN("[interval] ApplyRobotForRoom1 UNKONW ERR Account:%d enterRoom[%d] Err:%s, time = %I32u", it->second->Account(), nRoomId, TUPLE_ELE_C(tRet, 1), time(nullptr));
                    OnCliDisconnRoom(it->second, 0, nullptr, 0);
                    /*assert(false);*/
                    return 0;
                }
            }
            if (it->second->m_nWantGameId != 0) {
                n++;
                m_mapWantRoomRobot[nRoomId].insert(it->second); //添加进入房间失败的机器人 去 等待列表
            } else {
                assert(false);
            }
            nErr++; /*continue;*/
        }

        n++;
        //@zhuhangmin 移入RobotClient 以免丢包
        //UpdRobotClientToken(ECT_ROOM, it->second, true);

        UWL_INF("[interval] ApplyRobotForRoom1 enter room OK successfully Room[%d] Account:[%d], time = %I32u", nRoomId, it->second->Account(), time(nullptr));


        //@zhuhangmin 触发自动匹配
        NTF_GET_NEWTABLE lpNewTableInfo;
        tRet = it->second->SendGetNewTable(theRoom, m_thrdRoomNotify.ThreadId(), lpNewTableInfo);
        if (!TUPLE_ELE(tRet, 0)) {
            UWL_ERR("[interval] ApplyRobotForRoom1 Account:%d Room[%d] askNewTable Err:%s time = %I32u", it->second->Account(), nRoomId, TUPLE_ELE_C(tRet, 1), time(nullptr));
            OnCliDisconnRoom(it->second, 0, nullptr, 0);
            return 0;
        } else {
            if (!(it->second->IsGaming())) // 如果不在游戏服务器 则加入等待进入游戏服务器的列表
            {
                OnRoomRobotEnter(it->second, lpNewTableInfo.pp.nTableNO, lpNewTableInfo.pp.nChairNO, "");
            }
        }

        m_mapRoomRobot[nRoomId].insert(it->second);
        //}
    }
    return n;
}

//try to enter room amount of sepcific accounts robot
int32_t CRobotMgr::ApplyRobotForRoom(int32_t nGameId, int32_t nRoomId, TInt32Vec accounts) {
    if (!m_ConnHall) {
        UWL_ERR("ApplyRobotForRoom2 m_ConnHall is nil nGameId = %d, nRoomId = %d", nGameId, nRoomId);
        assert(false);
        return 0;
    }

    if (!m_ConnHall->IsConnected()) {
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
        CAutoLock lock(&m_csRobot);
        for (auto&& it = accounts.begin(); it != accounts.end(); it++) {
            auto&& it_ = m_mapAntRobot.find(*it); // 已经登陆大厅的机器人
            if (it_ == m_mapAntRobot.end()) continue;

            if (it_->second->RoomId() != 0)	continue;

            ROOM theRoom = {};
            GetRoomData(nRoomId, theRoom);

            tRet = it_->second->SendEnterRoom(theRoom, m_thrdRoomNotify.ThreadId());
            if (!TUPLE_ELE(tRet, 0)) {
                m_mapAcntSett[it_->second->Account()].logoned = false;
                UWL_WRN("ApplyRobotForRoom2 Account:%d enterRoom[%d] Err:%s", it_->second->Account(), nRoomId, TUPLE_ELE_C(tRet, 1));
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
                        OnCliDisconnRoom(it_->second, 0, nullptr, 0);
                        UWL_ERR("ApplyRobotForRoom2 ROOM_REQUEST_TIEM_OUT nGameId = %d, nRoomId = %d", nGameId, nRoomId);
                    } else if (strcmp(retStr, "ROOM_SOCKET_ERROR") == 0) {
                        UWL_ERR("ApplyRobotForRoom2 ROOM_SOCKET_ERROR nGameId = %d, nRoomId = %d", nGameId, nRoomId);
                        //assert(false);
                        OnCliDisconnRoom(it_->second, 0, nullptr, 0);
                    } else {
                        UWL_ERR("ApplyRobotForRoom2 UNKNOW ERROR  nGameId = %d, nRoomId = %d", nGameId, nRoomId);
                        assert(false);
                    }
                }
                if (it_->second->m_nWantGameId != 0)
                    m_mapWantRoomRobot[nRoomId].insert(it_->second);
                nErr++; continue;
            }

            n++;
            UpdRobotClientToken(ECT_ROOM, it_->second, true);
            m_mapRoomRobot[nRoomId].insert(it_->second);
            m_mapWantRoomRobot[nRoomId].erase(it_->second);


            //@zhuhangmin 触发自动匹配
            NTF_GET_NEWTABLE lpNewTableInfo;
            tRet = it_->second->SendGetNewTable(theRoom, m_thrdRoomNotify.ThreadId(), lpNewTableInfo);
            if (!TUPLE_ELE(tRet, 0)) {
                UWL_ERR("ApplyRobotForRoom1 Account:%d Room[%d] askNewTable Err:%s", it_->second->Account(), nRoomId, TUPLE_ELE_C(tRet, 1));
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

            TTokenId		nTokenID = pContext->lTokenID;
            TReqstId		nReqstID = pRequest->head.nRequest;

            auto client = GetRobotClient_ToknId(ECT_HALL, nTokenID);
            OnHallNotify(client, nReqstID, pRequest->pDataPtr, pRequest->nDataLen);

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

            TTokenId		nTokenID = pContext->lTokenID;
            TReqstId		nReqstID = pRequest->head.nRequest;

            auto client = GetRobotClient_ToknId(ECT_ROOM, nTokenID);
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

            TTokenId		nTokenID = pContext->lTokenID;
            TReqstId		nReqstID = pRequest->head.nRequest;

            auto client = GetRobotClient_ToknId(ECT_GAME, nTokenID);
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
            OnThrndDelyEnterGame(time(nullptr));
        }
    }
    UWL_INF(_T("EnterGame thread exiting. id = %d"), GetCurrentThreadId());
    return;
}

void	CRobotMgr::OnHallNotify(CRobotClient* client, TReqstId nReqId, void* pDataPtr, int32_t nSize) {
    if (client) {
        //UWL_INF("OnHallNotify client[%d] reqId:%d", client->Account(), nReqId);
    }
    switch (nReqId) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
            OnCliDisconnHall(client, nReqId, pDataPtr, nSize);
            break;

        case GR_GET_ROOMUSERS_OK:
            OnHallRoomUsersOK(nReqId, pDataPtr);
            return;
    }
}
void	CRobotMgr::OnRoomNotify(CRobotClient* client, TReqstId nReqId, void* pDataPtr, int32_t nSize) {
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
        UWL_INF("OnRoomNotify client[%d] reqId:%d", client->Account(), nReqId);
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
            OnCliDisconnRoom(client, nReqId, pDataPtr, nSize);
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
void	CRobotMgr::OnGameNotify(CRobotClient* client, TReqstId nReqId, void* pDataPtr, int32_t nSize) {
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
        UWL_INF("OnGameNotify client[%d] reqId:%d", client->Account(), nReqId);
    }
    switch (nReqId) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
            OnCliDisconnGame(client, nReqId, pDataPtr, nSize);
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
void	CRobotMgr::OnHallRoomUsersOK(TReqstId nReqId, void* pDataPtr) {
    CAutoLock lock(&m_csRobot);
    ITEM_COUNT* pItemCount = (ITEM_COUNT*) pDataPtr;
    ITEM_USERS* pItemUsers = (ITEM_USERS*) ((PBYTE) pDataPtr + sizeof(ITEM_COUNT));
    for (int32_t i = 0; i < pItemCount->nCount; i++, pItemUsers++) {
        for (auto&& it_ = mapRooms_.begin(); it_ != mapRooms_.end(); it_++) {
            if (it_->second.nRoomId == pItemUsers->nItemID)
                it_->second.nCurrNum = pItemUsers->nUsers;
        }
    }
}
void	CRobotMgr::OnRoomRobotEnter(CRobotClient* client, int32_t nTableNo, int32_t nChairNo, std::string sEnterWay) {
    client->m_nNeedEnterG = time(nullptr);
    client->m_nToTable = nTableNo;
    client->m_nToChair = nChairNo;
    client->m_sEnterWay = sEnterWay;
    AddWaitEnter(client);
    //UWL_INF("AddWaitEnter 已进入房间，等待进入游戏的机器人 %s: client[%d] room[%d].", sEnterWay.c_str(), client->Account(), client->RoomId());
}
void    CRobotMgr::OnCliDisconnHall(CRobotClient* client, TReqstId nReqId, void* pDataPtr, int32_t nSize) {
    UWL_ERR(_T("与大厅服务断开连接"));
    assert(false);
    m_ConnHall->DestroyEx();
    {
        CAutoLock lock(&m_csRobot);
        for (auto&& it = m_mapAcntSett.begin(); it != m_mapAcntSett.end(); it++) {
            it->second.logoned = false;
        }
    }
}
void    CRobotMgr::OnCliDisconnRoom(CRobotClient* client, TReqstId nReqId, void* pDataPtr, int32_t nSize) {
    CAutoLock lock(&m_csRobot);
    UWL_WRN("userid = %d disconnect room %d", client->Account(), client->RoomId());
    UpdRobotClientToken(ECT_ROOM, client, false);
    m_mapRoomRobot[client->RoomId()].erase(client);
    client->OnDisconnRoom();

}
void    CRobotMgr::OnCliDisconnGame(CRobotClient* client, TReqstId nReqId, void* pDataPtr, int32_t nSize) {
    CAutoLock lock(&m_csRobot);
    UWL_WRN("userid = %d disconnect game ", client->Account());
    UpdRobotClientToken(ECT_GAME, client, false);
    client->OnDisconnGame();
}

TTueRet CRobotMgr::RobotLogonHall(const int32_t& account) {
    CAutoLock lock(&m_csRobot);

    TAcntSettMap::iterator itRobot = m_mapAcntSett.end();

    // 指定账号 否则随机
    if (account != 0) {
        auto&& it = m_mapAcntSett.find(account); // robot setting
        if (it == m_mapAcntSett.end())
            return std::make_tuple(false, "使用无效的机器人账号");

        if (it->second.logoned)
            return std::make_tuple(true, "不可重复登录");
        itRobot = it;
    } else {
        //random
        for (int i = 0; i < 10; i++) {
            auto it = m_mapAcntSett.begin();
            auto randnum = rand();
            auto mapSize = m_mapAcntSett.size();
            auto num = randnum % mapSize;
            std::advance(it, num);
            if (it->second.logoned)			continue;
            itRobot = it; break;
        }

    }

    if (itRobot == m_mapAcntSett.end()) {

        if (account == 0) {
            //UWL_DBG("没有剩余机器人账号数据可用于登陆大厅");
        } else {
            UWL_ERR("没有找到机器人账号数据 account = %d", account);
            assert(false);
        }
        return std::make_tuple(true, "没有找到机器人账号数据可用于登陆大厅");
    }

    CRobotClient* client = nullptr;
    auto it_c = m_mapAntRobot.find(itRobot->second.account);
    if (it_c == m_mapAntRobot.end())
        client = new CRobotClient(itRobot->second);
    else
        client = it_c->second;

    LOGON_USER_V2  logonUser = {};
    logonUser.nUserID = client->Account();
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
    //strcpy_s(logonUser.szUsername, client->Account().c_str());
    strcpy_s(logonUser.szPassword, client->Password().c_str());

    TReqstId nResponse;
    LPVOID	 pRetData = NULL;
    uint32_t nDataLen = sizeof(logonUser);
    auto it = SendHallRequest(GR_LOGON_USER_V2, nDataLen, &logonUser, nResponse, pRetData);
    if (!TUPLE_ELE(it, 0)) {
        UWL_ERR("ACCOUNT = %d GR_LOGON_USER_V2 FAIL", client->Account());

        auto iter = m_mapUIdRobot.find(client->UserId());
        if (iter != m_mapUIdRobot.end()) {
            m_mapUIdRobot.erase(iter);
        }

        auto iterAnt = m_mapAntRobot.find(client->Account());
        if (iterAnt != m_mapAntRobot.end()) {
            m_mapAntRobot.erase(iterAnt);
        }
        SAFE_DELETE(client);
        return it;
    }

    if (!(nResponse == GR_LOGON_SUCCEEDED || nResponse == GR_LOGON_SUCCEEDED_V2)) {
        UWL_ERR("ACCOUNT = %d GR_LOGON_USER_V2 FAIL", client->Account());
        auto iter = m_mapUIdRobot.find(client->UserId());
        if (iter != m_mapUIdRobot.end()) {
            m_mapUIdRobot.erase(iter);
        }

        auto iterAnt = m_mapAntRobot.find(client->Account());
        if (iterAnt != m_mapAntRobot.end()) {
            m_mapAntRobot.erase(iterAnt);
        }
        SAFE_DELETE_ARRAY(pRetData);
        SAFE_DELETE(client);
        return std::make_tuple(false, std::to_string(nResponse));
    }
    itRobot->second.logoned = true;
    client->SetLogonData((LPLOGON_SUCCEED_V2) pRetData);

    m_mapUIdRobot[client->UserId()] = client;
    m_mapAntRobot[client->Account()] = client;

    SAFE_DELETE_ARRAY(pRetData);
    UWL_INF("account:%d userid:%d logon hall ok.", client->Account(), client->UserId());
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

    TReqstId nResponse;
    LPVOID	 pRetData = NULL;
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
        SAFE_DELETE_ARRAY(pRetData);
        return std::make_tuple(false, std::to_string(nResponse));
    }
    SetRoomData(nRoomId, (LPROOM) pRetData);
    SAFE_DELETE_ARRAY(pRetData);
    return std::make_tuple(true, ERR_OPERATE_SUCESS);
}
TTueRet	CRobotMgr::RobotGainDeposit(CRobotClient* client) {
    static TCHAR szHttpUrl[128];
    static int nResult = GetPrivateProfileString(_T("RobotReward"), _T("HttpUrl_G"), _T("null"), szHttpUrl, sizeof(szHttpUrl), g_szIniFile);
    if (0 == strcmp(szHttpUrl, "null"))
        return std::make_tuple(false, "RobotReward HttpUrl_G ini read faild");

    TCHAR szField[128] = {};
    sprintf_s(szField, "ActiveID_Game%d", client->m_nWantGameId);
    TCHAR szValue[128] = {};
    GetPrivateProfileString(_T("RobotReward"), szField, _T("null"), szValue, sizeof(szValue), g_szIniFile);
    if (0 == strcmp(szValue, "null"))
        return std::make_tuple(false, "RobotGainDeposit ActiveId ini read faild");

    Json::Value root;
    Json::FastWriter fast_writer;
    root["GameId"] = Json::Value(client->m_nWantGameId);
    root["ActId"] = Json::Value(szValue);
    root["UserId"] = Json::Value(client->UserId());
    root["UserNickName"] = Json::Value("Robot");
    root["Num"] = Json::Value(client->m_nGainAmount);
    root["Operation"] = Json::Value(1);
    root["Device"] = Json::Value(xyConvertIntToStr(client->m_nWantGameId) + "+" + xyConvertIntToStr(client->UserId()));
    root["Time"] = Json::Value((LONGLONG) time(NULL) * 1000);
    root["Sign"] = Json::Value(MD5String((root["ActId"].asString() + "|" + root["UserId"].asString() + "|" + root["Time"].asString() + "|" + ROBOT_APPLY_DEPOSIT_KEY).c_str()));

    CString strParam = fast_writer.write(root).c_str();
    CString strResult = ExecHttpRequestPost(szHttpUrl, strParam);
    Json::Reader reader;
    Json::Value _root;
    if (!reader.parse((LPCTSTR) strResult, _root))
        return std::make_tuple(false, "RobotGainDeposit Http result parse faild");
    if (_root["Code"].asInt() != 0) {
        UWL_ERR("m_Account = %d gain deposit fail, code = %d, strResult = %s", client->UserId(), _root["Code"].asInt(), strResult);
        //assert(false);
        return std::make_tuple(false, "RobotGainDeposit Http result Status false");
    }

    return std::make_tuple(true, ERR_OPERATE_SUCESS);
}

TTueRet	CRobotMgr::RobotBackDeposit(CRobotClient* client) {
    static TCHAR szHttpUrl[128];
    static int nResult = GetPrivateProfileString(_T("RobotReward"), _T("HttpUrl_C"), _T("null"), szHttpUrl, sizeof(szHttpUrl), g_szIniFile);
    if (0 == strcmp(szHttpUrl, "null"))
        return std::make_tuple(false, "RobotReward HttpUrl_C ini read faild");

    TCHAR szField[128] = {};
    sprintf_s(szField, "ActiveID_Game%d", client->m_nWantGameId);
    TCHAR szValue[128] = {};
    GetPrivateProfileString(_T("RobotReward"), szField, _T("null"), szValue, sizeof(szValue), g_szIniFile);
    if (0 == strcmp(szValue, "null"))
        return std::make_tuple(false, "RobotBackDeposit ActiveId ini read faild");

    Json::Value root;
    Json::FastWriter fast_writer;
    root["GameId"] = Json::Value(client->m_nWantGameId);
    root["ActId"] = Json::Value(szValue);
    root["UserId"] = Json::Value(client->UserId());
    root["UserNickName"] = Json::Value("Robot");
    root["Num"] = Json::Value(client->m_nBackAmount);
    root["Operation"] = Json::Value(2);
    root["Device"] = Json::Value(xyConvertIntToStr(client->m_nWantGameId) + "+" + xyConvertIntToStr(client->UserId()));
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

void    CRobotMgr::OnServerMainTimer(time_t nCurrTime) {
    OnTimerReconnectHall(nCurrTime);
    OnTimerSendHallPluse(nCurrTime);
    OnTimerSendRoomPluse(nCurrTime);
    OnTimerSendGamePluse(nCurrTime);
    OnTimerCtrlRoomActiv(nCurrTime);
    OnTimerUpdateDeposit(nCurrTime);
}
bool	CRobotMgr::OnTimerReconnectHall(time_t nCurrTime) {
    if (!m_ConnHall) {
        UWL_ERR("SendHallRequest OnTimerReconnectHall nil ERR_CONNECT_NOT_EXIST");
        assert(false);
        return false;
    }


    if (m_ConnHall->IsConnected()) return true;

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
        CAutoLock lock(&m_csRobot);
        for (auto&& it = m_mapAcntSett.begin(); it != m_mapAcntSett.end(); it++) {
            if (!it->second.logoned) continue;

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
    if (!m_ConnHall) {
        UWL_ERR("SendHallRequest OnTimerSendHallPluse nil ERR_CONNECT_NOT_EXIST nReqId");
        assert(false);
        return;
    }


    if (!m_ConnHall->IsConnected()) {
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

        TReqstId nResponse;
        LPVOID	 pRetData = NULL;
        uint32_t nDataLen = sizeof(HALLUSER_PULSE);
        (void) SendHallRequest(GR_HALLUSER_PULSE, nDataLen, &hp, nResponse, pRetData, false);
    }
}
void    CRobotMgr::OnTimerSendRoomPluse(time_t nCurrTime) {
#define MAIN_SEND_ROOM_PULSE_GAP_TIME (1*60+1) // 1分钟1秒
    static time_t	sLastSendRPulseGapTime = nCurrTime;
    if (nCurrTime - sLastSendRPulseGapTime >= MAIN_SEND_ROOM_PULSE_GAP_TIME)
        sLastSendRPulseGapTime = nCurrTime;
    else return;
    {
        CAutoLock lock(&m_csRobot);
        for (auto&& it = m_mapUIdRobot.begin(); it != m_mapUIdRobot.end(); it++) {
            if (it->second->RoomId() == 0)	continue;

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
        CAutoLock lock(&m_csRobot);
        for (auto&& it = m_mapUIdRobot.begin(); it != m_mapUIdRobot.end(); it++) {
            if (!it->second->IsGaming())	continue;

            it->second->SendGamePulse();
        }
    }
}
void    CRobotMgr::OnTimerCtrlRoomActiv(time_t nCurrTime) {
    if (!m_ConnHall) {
        UWL_ERR("SendHallRequest OnTimerCtrlRoomActiv nil ERR_CONNECT_NOT_EXIST");
        assert(false);
        return;
    }

    if (!m_ConnHall->IsConnected()) {
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
        CAutoLock lock(&m_csRobot);
        for (auto&& it_2 = mapRooms_.begin(); it_2 != mapRooms_.end(); it_2++) {
            if (it_2->second.nCtrlMode == EACM_Hold) {
                gr.nRoomID[gr.nRoomCount] = it_2->second.nRoomId;

                //@zhuhangmin
                auto curRoomUsers = it_2->second.nCurrNum;
                auto curRoomID = it_2->second.nRoomId;
                //auto curInterval = GetIntervalTime(curRoomUsers);
                //zhuhangmin

                gr.nRoomCount++;

                //zhuhangmin 计算处于各种“状态”的机器人
                int robotInSeating = 0;
                int robotInWaiting = 0;
                int robotInPlaying = 0;
                int unexpectedState = 0;
                for (auto&& it_3 = m_mapRoomRobot[it_2->second.nRoomId].begin(); it_3 != m_mapRoomRobot[it_2->second.nRoomId].end(); it_3++) {
                    int state = (*it_3)->GetPlayerRoomStatus();
                    if (state == ROOM_PLAYER_STATUS_PLAYING) {
                        robotInPlaying++;
                        //UWL_DBG("m_Account = %d is playing", (*it_3)->Account());

                    } else if (state == ROOM_PLAYER_STATUS_WAITING) {
                        robotInWaiting++;
                        UWL_DBG("[ROOM STATUS] m_Account = %d is waitting", (*it_3)->Account());

                    }
                    //else if (state == ROOM_PLAYER_STATUS_SEATED){
                    //	robotInSeating++;
                    //	//UWL_DBG("m_Account = %d is seating", (*it_3)->Account());

                    //}
                    //else if (state == ROOM_PLAYER_STATUS_WALKAROUND){
                    //	robotInSeating++;
                    //	//UWL_DBG("m_Account = %d is walking", (*it_3)->Account());

                    //}
                    //else if (state == PLAYER_STATUS_OFFLINE){
                    //	robotInSeating++;
                    //	UWL_DBG("m_Account = %d is offline", (*it_3)->Account());
                    //}
                    //else{
                    //	unexpectedState++;
                    //	UWL_WRN("m_Account = %d is unexpected state [%d]", (*it_3)->Account(), state);
                    //}

                }
                UWL_INF("[ROOM STATUS] room id = %d , already in room %d,  need in room %d,  in playing %d, in waitting %d", it_2->second.nRoomId, m_mapRoomRobot[it_2->first].size(), m_mapWantRoomRobot[it_2->first].size(), robotInPlaying, robotInWaiting);
                //if (m_mapRoomRobot[it_2->first].size() + m_mapWantRoomRobot[it_2->first].size() - robotInPlaying < it_2->second.nCtrlVal)
                //if (robotInWaiting < it_2->second.nCtrlVal)
                //    roomNeedApply[it_2->first] = it_1->first;// roomid:gameid
            }
            if (it_2->second.nCtrlMode == EACM_Scale) {
                gr.nRoomID[gr.nRoomCount] = it_2->second.nRoomId;
                gr.nRoomCount++;

                auto room_robot_num = m_mapRoomRobot[it_2->first].size() + m_mapWantRoomRobot[it_2->first].size();
                auto scale_user_num = (size_t) ((it_2->second.nCurrNum - room_robot_num)*it_2->second.nCtrlVal*0.01f);
                /*if (it_2->second.nCurrNum > room_robot_num && room_robot_num < scale_user_num)
                    roomNeedApply[it_2->first] = it_1->first;*/
            }
        }
    }

    TRobotCliSet vecWantClient;
    {
        CAutoLock lock(&m_csRobot);
        for (auto&& it = m_mapWantRoomRobot.begin(); it != m_mapWantRoomRobot.end(); it++) {
            for (auto&& itCli = it->second.begin(); itCli != it->second.end(); itCli++) {
                vecWantClient.insert(*itCli);
            }
        }
    }
    for (auto&& it : vecWantClient) {
        ApplyRobotForRoom(it->m_nWantGameId, it->m_nWantRoomId, TInt32Vec{it->Account()});
    }

    for (auto&& it : roomNeedApply) {
        ApplyRobotForRoom(it.second, it.first, 1);
        //ApplyRobotForRoom(it.second, it.first, m_mapAcntSett.size());
    }
    if (gr.nRoomCount > 0) {
        TReqstId nResponse;
        LPVOID	 pRetData = NULL;
        uint32_t nDataLen = sizeof(GET_ROOMUSERS) - sizeof(int)*MAX_QUERY_ROOMS + gr.nRoomCount*sizeof(int);
        (void) SendHallRequest(GR_GET_ROOMUSERS, nDataLen, &gr, nResponse, pRetData, false);
    }
}
void    CRobotMgr::OnTimerUpdateDeposit(time_t nCurrTime) {
    if (!m_ConnHall) {
        UWL_ERR("SendHallRequest OnTimerCtrlRoomActiv nil ERR_CONNECT_NOT_EXIST nReqId");
        assert(false);
        return;
    }


    if (!m_ConnHall->IsConnected()) {
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
        CAutoLock lock(&m_csRobot);
        for (auto&& it = m_mapUIdRobot.begin(); it != m_mapUIdRobot.end(); it++) {
            if (it->second->RoomId() != 0)	continue;

            if (it->second->IsGaming())		continue;

            if (it->second->m_bGainDeposit) {
                it->second->m_bGainDeposit = false;
                auto&& tRet = RobotGainDeposit(it->second);
                if (!TUPLE_ELE(tRet, 0)) {
                    UWL_WRN("Account:%d GainDeposit Err:%s", it->second->Account(), TUPLE_ELE_C(tRet, 1));
                    continue;
                }
                nCount++;
            }
            if (it->second->m_bBackDeposit) {
                it->second->m_bBackDeposit = false;
                auto&& tRet = RobotBackDeposit(it->second);
                if (!TUPLE_ELE(tRet, 0)) {
                    UWL_WRN("Account:%d BackDeposit Err:%s", it->second->Account(), TUPLE_ELE_C(tRet, 1));
                    continue;
                }
                nCount++;
            }
            if (nCount >= UPDATE_CLIENTNUM_PRE_ONCE)
                break;
        }
    }
}
void    CRobotMgr::OnThrndDelyEnterGame(time_t nCurrTime) {
    do {
        CRobotClient* pRoboter = GetWaitEnter();
        if (pRoboter == nullptr)	return;

        ROOM theRoom = {};
        if (!GetRoomData(pRoboter->RoomId(), theRoom)) {
            UWL_ERR("Room %s: client[%d] room[%d] not found.", pRoboter->m_sEnterWay.c_str(), pRoboter->Account(), pRoboter->RoomId());
            //assert(false);
            continue;
        }
        stRobotUnit unit;
        if (!GetRobotSetting(pRoboter->Account(), unit)) {
            UWL_ERR("Room %s: client[%d] RobotUnit not found.", pRoboter->m_sEnterWay.c_str(), pRoboter->Account());
            assert(false);
            continue;
        }

        auto begTime = GetTickCount();
        UWL_DBG("[PROFILE] 0 SendEnterGame timestamp = %ld", GetTickCount());
        auto ret = pRoboter->SendEnterGame(theRoom, m_thrdGameNotify.ThreadId(), unit.nickName, unit.portraitUrl, pRoboter->m_nToTable, pRoboter->m_nToChair);
        UWL_DBG("[PROFILE] 7 SendEnterGame timestamp = %ld", GetTickCount());
        if (!TUPLE_ELE(ret, 0)) {
            UWL_WRN("Room %s: client[%d] enter game faild:%s.", pRoboter->m_sEnterWay.c_str(), pRoboter->Account(), TUPLE_ELE_C(ret, 1));
            UWL_WRN("Room %s: client[%d] enter game faild try to reset the socket", pRoboter->m_sEnterWay.c_str(), pRoboter->Account());
            UWL_DBG("[interval] OnThrndDelyEnterGame client[%d] end fail, time = %I32u", pRoboter->Account(), time(nullptr));
            OnCliDisconnGame(pRoboter, 0, nullptr, 0);
            continue;
        }
        //UpdRobotClientToken(ECT_GAME, pRoboter, true); @zhuhangmin 移入RobotClient
        auto endTime = GetTickCount();
        UWL_DBG("[interval] Room %s: client[%d] Dely Enter Game OK end successfully. time %d ", pRoboter->m_sEnterWay.c_str(), pRoboter->Account(), time(nullptr));
        UWL_DBG("[interval] [PROFILE] ENTER GAME cost time = %ld", endTime - begTime);

        UWL_INF("[STATUS] client = %d status waitting", pRoboter->Account());


    } while (true);
}