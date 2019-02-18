#include "stdafx.h"
#include "Robot.h"
#include "Main.h"
#include "RobotReq.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

Robot::Robot(const RobotSetting& robot) {
    userid_ = robot.account;
    m_Password = robot.password;
}
Robot::~Robot() {}

void Robot::OnDisconnRoom() {
    std::lock_guard<std::mutex> lg(m_mutex);
    m_nRoomId = 0;
    m_ConnRoom->DestroyEx();
    m_playerRoomStatus = PLAYER_STATUS_OFFLINE; // 退出房间后 恢复默认状态
    UWL_INF("[STATUS] account:%d userid:%d status [offline] ", GetUserID(), GetUserID());
    m_bRunGame = false; //@zhuhangmin 20181129 用户房间服务器崩溃情况
}
void Robot::OnDisconnGame() {
    std::lock_guard<std::mutex> lg(m_mutex);
    m_bRunGame = false;
    m_ConnGame->DestroyEx();
}
bool Robot::ConnectRoom(const std::string& strIP, const int32_t nPort, uint32_t nThrdId) {
    CAutoLock lock(&m_csConnRoom);

    m_ConnRoom->InitKey(KEY_HALL, ENCRYPT_AES, 0);
    std::string sIp = (g_useLocal) ? "127.0.0.1" : strIP;
    //由于目前网络库不支持IPV6，所以添加配置项，可以选定本地地址
    if (!m_ConnRoom->Create(sIp.c_str(), nPort, 5, 0, nThrdId, 0, GetHelloData(), GetHelloLength())) {
        UWL_ERR("ConnectRoom Faild! IP:%s Port:%d", sIp.c_str(), nPort);
        //assert(false);
        return false;
    }

    return true;
}
bool Robot::ConnectGame(const std::string& strIP, const int32_t nPort, uint32_t nThrdId) {

    {
        CAutoLock lock(&m_csConnGame);

        m_ConnGame->InitKey(KEY_GAMESVR_2_0, ENCRYPT_AES, 0);
        //由于目前网络库不支持IPV6，所以添加配置项，可以选定本地地址
        std::string sIp = (g_useLocal) ? "127.0.0.1" : strIP;
        if (!m_ConnGame->Create(sIp.c_str(), nPort, 5, 0, nThrdId, 0, GetHelloData(), GetHelloLength())) {
            UWL_ERR("ConnectGame Faild! IP:%s Port:%d", sIp.c_str(), nPort);
            //assert(false);
            return false;
        }

        ////TODO FIXME DECOUPLE WITH MANAGER
        ////@zhuhangmin 20181129 立即加上token信息 以免丢包
    }


    return true;
}
TTueRet Robot::SendRoomRequest(TReqstId nReqId, uint32_t& nDataLen, void *pData, TReqstId &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho /*= true*/, uint32_t wait_ms /*= REQ_TIMEOUT_INTERVAL*/) {
    if (!m_ConnRoom) {
        UWL_ERR("m_ConnRoom nil");
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_NOT_EXIST);
    }


    if (!m_ConnRoom->IsConnected()) {
        UWL_ERR("m_ConnRoom not connected");
        //assert(false);
        return std::make_tuple(false, ERR_CONNECT_DISABLE);
    }


    CONTEXT_HEAD	Context = {};
    REQUEST			Request = {};
    REQUEST			Response = {};
    Context.hSocket = m_ConnRoom->GetSocket();
    Context.lSession = 0;
    Context.bNeedEcho = bNeedEcho;
    Request.head.nRepeated = 0;
    Request.head.nRequest = nReqId;
    Request.nDataLen = nDataLen;
    Request.pDataPtr = pData;//////////////

    BOOL bTimeOut = FALSE, bResult = TRUE;
    {
        CAutoLock lock(&m_csConnRoom);
        bResult = m_ConnRoom->SendRequest(&Context, &Request, &Response, bTimeOut, wait_ms);
    }

    if (!bResult)///if timeout or disconnect 
    {
        //assert(false);
        return std::make_tuple(false, (bTimeOut ? ERR_SENDREQUEST_TIMEOUT : ERR_OPERATE_FAILED));
    }


    nDataLen = Response.nDataLen;
    nRespId = Response.head.nRequest;
    pRetData.reset(Response.pDataPtr);


    if (nRespId == GR_ERROR_INFOMATION) {
        UWL_ERR("GR_ERROR_INFOMATION");
        assert(false);
        CHAR info[512] = {};
        sprintf_s(info, "%s", pRetData.get());
        nDataLen = 0;

        return std::make_tuple(false, info);
    }
    return std::make_tuple(true, ERR_OPERATE_SUCESS);
}

TTueRet Robot::SendGameRequest(TReqstId nReqId, uint32_t& nDataLen, void *pData, TReqstId &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho /*= true*/, uint32_t wait_ms /*= REQ_TIMEOUT_INTERVAL*/) {
    if (!m_ConnGame) {
        UWL_ERR("m_ConnGame is nil");
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_NOT_EXIST);
    }


    if (!m_ConnGame->IsConnected()) {
        UWL_ERR("m_ConnGame not connected");
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_DISABLE);
    }


    CONTEXT_HEAD	Context = {};
    REQUEST			Request = {};
    REQUEST			Response = {};
    Context.hSocket = m_ConnGame->GetSocket();
    Context.lSession = 0;
    Context.bNeedEcho = bNeedEcho;
    Request.head.nRepeated = 0;
    Request.head.nRequest = nReqId;
    Request.nDataLen = nDataLen;
    Request.pDataPtr = pData;//////////////

    BOOL bTimeOut = FALSE, bResult = TRUE;
    {
        CAutoLock lock(&m_csConnGame);
        bResult = m_ConnGame->SendRequest(&Context, &Request, &Response, bTimeOut, wait_ms);
    }

    if (!bResult)///if timeout or disconnect 
    {
        UWL_ERR("game send quest fail");
        //assert(false);
        return std::make_tuple(false, (bTimeOut ? ERR_SENDREQUEST_TIMEOUT : ERR_OPERATE_FAILED));
    }


    nDataLen = Response.nDataLen;
    nRespId = Response.head.nRequest;
    pRetData.reset(Response.pDataPtr);

    if (nRespId == GR_ERROR_INFOMATION) {
        CHAR info[512] = {};
        sprintf_s(info, "%s", pRetData.get());
        nDataLen = 0;
        return std::make_tuple(false, info);
    }
    if (0 == nRespId) {
        //assert(false);
        UWL_ERR("enter game respId = 0");
        CHAR info[512] = {};
        sprintf_s(info, "%s", pRetData.get());
        nDataLen = 0;
        return std::make_tuple(false, info);
    }
    return std::make_tuple(true, ERR_OPERATE_SUCESS);
}

TTueRet Robot::SendEnterRoom(const ROOM& room, uint32_t nNofifyThrId) {
    std::lock_guard<std::mutex> lg(m_mutex);
    if (!m_ConnRoom) {
        UWL_ERR("m_ConnRoom nil");
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_NOT_EXIST);
    }

    if (!m_ConnRoom->IsConnected()) {

        if (!ConnectRoom(room.szGameIP, room.nPort == 0 ? 30629 : room.nPort, nNofifyThrId)) {
            UWL_ERR("ConnectRoom connect fail");
            //assert(false);
            return std::make_tuple(false, ERR_CONNECT_DISABLE);
        }
    }

    ENTER_ROOM  er = {};
    er.dwEnterFlags = FLAG_ENTERROOM_INTER;

    er.nUserID = m_LogonData.nUserID;
    er.nAreaID = room.nAreaID;
    er.nGameID = room.nGameID;
    er.nRoomID = room.nRoomID;
    er.nExeMajorVer = 1111111; // 赋值
    er.nExeMinorVer = 2222222; // 赋值

    lstrcpy(er.szUniqueID, m_LogonData.szUniqueID);
    xyGetHardID(er.szHardID);  //硬件ID 
    xyGetVolumeID(er.szVolumeID);
    xyGetMachineID(er.szMachineID);
    er.dwScreenXY = MAKELONG(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    HDC hDC = ::GetDC(NULL);
    er.dwPixelsXY = MAKELONG(GetDeviceCaps(hDC, LOGPIXELSX), GetDeviceCaps(hDC, LOGPIXELSY));
    ::ReleaseDC(NULL, hDC);

    int nClientPort = 0;
    UwlGetSockPort(m_ConnRoom->GetSocket(), nClientPort);
    er.dwClientPort = MAKELONG(nClientPort, 0);

    int nServerPort = 0;
    UwlGetPeerPort(m_ConnRoom->GetSocket(), nServerPort);
    er.dwServerPort = MAKELONG(nServerPort, 0);

    UwlGetSockAddr(m_ConnRoom->GetSocket(), er.stClientSockIP);
    UwlGetPeerAddr(m_ConnRoom->GetSocket(), er.stRemoteSockIP);

    er.stClientLANIP = er.stClientSockIP;

    if (er.stClientLANIP.v4_addr.sin_addr.s_addr == IP127001 || er.stClientLANIP.v4_addr.sin_addr.s_addr == 0)
        er.stClientLANIP.v4_addr.sin_addr.s_addr = GetLocalIPByRemote((LPTSTR) room.szGameIP, room.nPort);
    if (er.stClientLANIP.v4_addr.sin_addr.s_addr == IP127001 || er.stClientLANIP.v4_addr.sin_addr.s_addr == 0)
        er.stClientLANIP.v4_addr.sin_addr.s_addr = xyGetMachineIP();
    if (er.stClientLANIP.v4_addr.sin_addr.s_addr == IP127001 || er.stClientLANIP.v4_addr.sin_addr.s_addr == 0)
        er.stClientLANIP.v4_addr.sin_addr.s_addr = xydyGetEstablishedLocalIP();

    lstrcpyn(er.szPhysAddr, xydyGetPhysAddrByIP(er.stClientLANIP.v4_addr.sin_addr.s_addr), MAX_PHYSADDR_LEN);
    er.dwClientMask = xydyGetMaskByIP(er.stClientLANIP.v4_addr.sin_addr.s_addr);
    //er.stClientGateway; 默认值
    er.dwClientDNS = xydyGetPrimaryDNSByIP(er.stClientLANIP.v4_addr.sin_addr.s_addr);

    TReqstId nResponse;
    //LPVOID	 pRetData = NULL;
    std::shared_ptr<void> pRetData;
    uint32_t nDataLen = sizeof(ENTER_ROOM);
    auto it = SendRoomRequest(GR_ENTER_ROOM, nDataLen, &er, nResponse, pRetData, true, 4000);
    if (!TUPLE_ELE(it, 0))
        return it;

    if (nResponse != GR_ENTER_ROOM_OK) {
        std::string ret = std::to_string(nResponse);
        if (nResponse == GR_DEPOSIT_NOTENOUGH) {
            int64_t nPlayerDeposit = *(int64_t*) pRetData.get();
            int64_t nMinRoomDeposit = *(int64_t*) ((PBYTE) pRetData.get() + sizeof(int64_t));
            int64_t nDiffDeposit = nMinRoomDeposit - nPlayerDeposit;
            m_nWantRoomId = room.nRoomID;
            m_bGainDeposit = true;
            m_nGainAmount = 200000;
            ret = "GR_DEPOSIT_NOTENOUGH";
        }
        if (nResponse == GR_DEPOSIT_OVERFLOW) {
            int64_t nPlayerDeposit = *(int64_t*) pRetData.get();
            int64_t nMaxRoomDeposit = *(int64_t*) ((PBYTE) pRetData.get() + sizeof(int64_t));
            int64_t nDiffDeposit = nPlayerDeposit - nMaxRoomDeposit;
            m_nWantRoomId = room.nRoomID;
            m_bBackDeposit = true;
            m_nBackAmount = nDiffDeposit*1.2;
            ret = "GR_DEPOSIT_OVERFLOW";
        }

        if (nResponse != GR_DEPOSIT_NOTENOUGH_EX) {
            UWL_ERR("account:%d userid:%d enter room[%d] of game[%d] ERROR , ROOM nResponse = [%d] ", GetUserID(), GetUserID(), room.nRoomID, room.nGameID, nResponse);
            if (nResponse == GR_ROOM_NEED_DXXW_EX) {
                ret = "ROOM_NEED_DXXW";
                return std::make_tuple(false, ret);
            } else {
                UWL_ERR("send room request fail nResponse = %d", nResponse);
            }
        }

        return std::make_tuple(false, ret);
    }
    m_EnterRoomData = *(LPENTER_ROOM_OK) pRetData.get();
    m_nRoomId = room.nRoomID;
    if (room.nRoomID == 0) {
        UWL_ERR("account = %d, room.nRoomID = 0 ???", userid_, room.nRoomID);
        //assert(false);
        return std::make_tuple(false, "room id 0");
    }
    int nPlayerMinSize = sizeof(PLAYER) - sizeof(PLAYER_EXTEND);
    LPPLAYER  lpPlayer = (LPPLAYER) (PBYTE(pRetData.get()) + sizeof(ENTER_ROOM_OK));
    for (int i = 0; i < m_EnterRoomData.nPlayerCount; i++) {
        if (lpPlayer->nUserID == GetUserID()) {
            memcpy(&m_PlayerData, lpPlayer, nPlayerMinSize);
        }
        /////////////////////////////////////////// 
        lpPlayer = (LPPLAYER) ((PBYTE) lpPlayer + nPlayerMinSize); //服务器发过来的是nPlayerMinSize长度
    }
    m_nWantRoomId = 0;

    m_playerRoomStatus = ROOM_PLAYER_STATUS_WALKAROUND;
    UWL_INF("[STATUS] account:%d userid:%d room[%d] of game[%d] status [walking] ", GetUserID(), GetUserID(), room.nRoomID, room.nGameID);

    return std::make_tuple(true, ERR_OPERATE_SUCESS);
}
TTueRet	Robot::SendEnterGame(const ROOM& room, uint32_t nNofifyThrId, std::string sNick, std::string sPortr, int nTableNo, int nChairNo) {
    std::lock_guard<std::mutex> lg(m_mutex);
    UWL_DBG("[PROFILE] 1 SendEnterGame timestamp = %ld", GetTickCount());
    if (!m_ConnGame) {
        UWL_ERR("m_ConnGame is nil");
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_NOT_EXIST);
    }
    UWL_DBG("[PROFILE] 2 SendEnterGame timestamp = %ld", GetTickCount());

    if (!m_ConnGame->IsConnected()) {
        UWL_DBG("[PROFILE] 3 SendEnterGame timestamp = %ld", GetTickCount());
        if (!ConnectGame(room.szGameIP, room.nGamePort, nNofifyThrId)) {
            UWL_ERR("m_ConnGame not connect fail ip = %s, port = %d", room.szGameIP, room.nGamePort);
            //assert(false);
            return std::make_tuple(false, ERR_CONNECT_DISABLE);
        }
    }

    UWL_DBG("[PROFILE] 4 SendEnterGame timestamp = %ld", GetTickCount());
    // enter_game
    ENTER_GAME_EX enter = {};
    enter.nUserID = GetUserID();
    enter.nUserType = m_LogonData.nUserType;

    enter.nGameID = room.nGameID;
    enter.nRoomID = room.nRoomID;
    enter.nTableNO = nTableNo; // 赋值
    enter.nChairNO = nChairNo; // 赋值

    xyGetHardID(enter.szHardID);
    enter.dwLookOn = 0;
    enter.dwUserConfigs = 0;
    //enter.nRoomTokenID = m_ConnRoom->GetTokenID();
    enter.nRoomTokenID = m_EnterRoomData.nRoomTokenID;

    // solo_player
    SOLO_PLAYER splayer = {};
    splayer.nUserID = GetUserID();
    splayer.nUserType = m_LogonData.nUserType;
    splayer.nStatus = ROOM_PLAYER_STATUS_SEATED;
    splayer.nTableNO = nTableNo;
    splayer.nChairNO = nChairNo;
    splayer.nNickSex = m_LogonData.nNickSex;
    splayer.nPortrait = m_LogonData.nPortrait;
    splayer.nNetSpeed = 10;
    splayer.nClothingID = m_LogonData.nClothingID;

    splayer.nDeposit = m_PlayerData.nDeposit;
    splayer.nPlayerLevel = m_PlayerData.nPlayerLevel;
    splayer.nScore = m_PlayerData.nScore;
    splayer.nBreakOff = m_PlayerData.nBreakOff;
    splayer.nWin = m_PlayerData.nWin;
    splayer.nLoss = m_PlayerData.nLoss;
    splayer.nStandOff = m_PlayerData.nStandOff;
    splayer.nBout = m_PlayerData.nBout;
    splayer.nTimeCost = m_PlayerData.nTimeCost;

    lstrcpy(splayer.szUsername, (LPCTSTR) xyAnsiToUtf8(sNick.c_str()));
    //lstrcpy(splayer.szNickName, (LPCTSTR)xyAnsiToUtf8(sNick.c_str()));
    lstrcpy(splayer.szPortrait, sPortr.c_str());
    // build datas
    uint32_t nDataLen = sizeof(ENTER_GAME_EX) + sizeof(SOLO_PLAYER);
    //BYTE * pSendData = new BYTE[nDataLen] {};
    auto pSendData = std::make_unique<BYTE>(nDataLen);
    memcpy(pSendData.get(), &enter, sizeof(enter));
    memcpy(pSendData.get() + sizeof(enter), &splayer, sizeof(splayer));
    if (!IS_BIT_SET(room.dwConfigs, RC_SOLO_ROOM))
        nDataLen = sizeof(ENTER_GAME_EX);

    TReqstId nResponse;
    //LPVOID	 pRetData = NULL;
    std::shared_ptr<void> pRetData;
    //uint32_t nDataLen = 0;


    UWL_DBG("[PROFILE] 5 SendEnterGame timestamp = %ld", GetTickCount());
    auto it = SendGameRequest(GR_ENTER_GAME_EX, nDataLen, pSendData.get(), nResponse, pRetData, true);
    m_playerRoomStatus = ROOM_PLAYER_STATUS_WAITING;
    UWL_INF("[STATUS] account:%d userid:%d status [walking] ", GetUserID(), GetUserID());
    if (!TUPLE_ELE(it, 0)) {
        UWL_ERR("GR_ENTER_GAME_EX fail userid = %d", GetUserID());
        //assert(false);
        return it;
    }

    if (nResponse == GR_RESPONE_ENTER_GAME_OK) {
        m_bRunGame = true;
        UWL_DBG("[PROFILE] 6 SendEnterGame timestamp = %ld", GetTickCount());
        return std::make_tuple(true, "GR_RESPONE_ENTER_GAME_OK");
    } else if (nResponse == GR_RESPONE_ENTER_GAME_DXXW) {
        m_bRunGame = true;
        return std::make_tuple(true, "GR_RESPONE_ENTER_GAME_DXXW");
    } else if (nResponse == GR_RESPONE_ENTER_GAME_IDLE)		//游戏中，空闲玩家进入游戏
    {
        m_bRunGame = true;
        return std::make_tuple(true, "GR_RESPONE_ENTER_GAME_IDLE");
    } else {
        UWL_ERR("UNHANDLE RESPOSE %d = ", nResponse);
    }

    return std::make_tuple(false, std::to_string(nResponse));
}

//@zhuhangmin
TTueRet Robot::SendGetNewTable(const ROOM& room, uint32_t nNofifyThrId, NTF_GET_NEWTABLE& lpNewTableInfo) {
    std::lock_guard<std::mutex> lg(m_mutex);
    if (!m_ConnRoom) {
        UWL_ERR("m_ConnRoom nil");
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_NOT_EXIST);
    }

    if (!m_ConnRoom->IsConnected()) {
        UWL_ERR("room not connected");
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_DISABLE);
    }

    GET_NEWTABLE gn = {};
    gn.nUserID = m_LogonData.nUserID;
    gn.nRoomID = room.nRoomID;
    gn.nAreaID = room.nAreaID;
    gn.nGameID = room.nGameID;
    gn.nTableNO = 0;//?
    gn.nChairNO = 0;//?
    gn.nIPConfig = 0;//?
    gn.nBreakReq = 0;//?
    gn.nSpeedReq = 0;//?
    gn.nMinScore = room.nMinScore;
    gn.nMinDeposit = room.nMinDeposit;
    gn.nWaitSeconds = 0;//?
    gn.nNetDelay = 0;//?

    TReqstId nResponse;
    //LPVOID	 pRetData = NULL;
    std::shared_ptr<void> pRetData;
    uint32_t nDataLen = sizeof(GET_NEWTABLE);
    auto it = SendRoomRequest(MR_GET_NEWTABLE, nDataLen, &gn, nResponse, pRetData, true, 4000);
    if (!TUPLE_ELE(it, 0)) {
        UWL_ERR("send get newtable fail");
        return it;
    }


    if (nResponse != UR_OPERATE_SUCCEEDED) {
        std::string ret = std::to_string(nResponse);
        /*if (nResponse == GR_DEPOSIT_NOTENOUGH)
        {

        }*/

        return std::make_tuple(false, ret);
    }

    lpNewTableInfo = *(LPNTF_GET_NEWTABLE) pRetData.get();

    m_playerRoomStatus = ROOM_PLAYER_STATUS_SEATED;
    UWL_INF("[STATUS] account:%d userid:%d room[%d] of game[%d] status [seated]", GetUserID(), GetUserID(), room.nRoomID, room.nGameID);
    UWL_INF("account:%d userid:%d get table [%d] of chair[%d] ok.", GetUserID(), GetUserID(), lpNewTableInfo.pp.nTableNO, lpNewTableInfo.pp.nChairNO);
    return std::make_tuple(true, ERR_OPERATE_SUCESS);
}

TTueRet	Robot::SendRoomPulse() {
    if (!m_ConnRoom) {
        UWL_ERR("m_ConnRoom nil");
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_NOT_EXIST);
    }
    /*
        if (!m_ConnRoom->IsConnected()) {
        UWL_ERR("room not connected");
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_DISABLE);
        }

        ROOMUSER_PULSE rp = {};
        rp.nUserID = GetUserID();
        rp.nRoomID = GetRoomID();

        TReqstId nResponse;
        LPVOID	 pRetData = NULL;
        uint32_t nDataLen = sizeof(ROOMUSER_PULSE);
        return SendRoomRequest(GR_ROOMUSER_PULSE, nDataLen, &rp, nResponse, pRetData, false);*/
}
TTueRet	Robot::SendGamePulse() {
    if (!m_ConnGame) {
        UWL_ERR("m_ConnGame is nil");
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_NOT_EXIST);
    }


    /*    if (!m_ConnGame->IsConnected()) {
            UWL_ERR("m_ConnGame not connected");
            assert(false);
            return std::make_tuple(false, ERR_CONNECT_DISABLE);
            }


            GAME_PULSE gp = {};
            gp.nUserID = GetUserID();
            gp.dwAveDelay = 11;
            gp.dwMaxDelay = 22;

            TReqstId nResponse;
            LPVOID	 pRetData = NULL;
            uint32_t nDataLen = sizeof(GAME_PULSE);
            return SendRoomRequest(GR_GAME_PULSE, nDataLen, &gp, nResponse, pRetData, false)*/;
}
