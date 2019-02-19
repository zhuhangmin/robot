#include "stdafx.h"
#include "Robot.h"
#include "Main.h"
#include "RobotReq.h"
#include "common_func.h"
#include "RobotUitls.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

Robot::Robot(const RobotSetting& robot) {
    userid_ = robot.account;
    m_Password = robot.password;
}
Robot::~Robot() {}

void Robot::OnDisconnRoom() {
    std::lock_guard<std::mutex> lock(mutex_);
    roomid_ = 0;
    connection_room_->DestroyEx();
    m_playerRoomStatus = PLAYER_STATUS_OFFLINE; // 退出房间后 恢复默认状态
    UWL_INF("[STATUS] account:%d userid:%d status [offline] ", GetUserID(), GetUserID());
    m_bRunGame = false; //@zhuhangmin 20181129 用户房间服务器崩溃情况
}
void Robot::OnDisconnGame() {
    std::lock_guard<std::mutex> lock(mutex_);
    m_bRunGame = false;
    game_connection_->DestroyEx();
}
bool Robot::ConnectRoomWithLock(const std::string& strIP, const int32_t nPort, uint32_t nThrdId) {
    connection_room_->InitKey(KEY_HALL, ENCRYPT_AES, 0);
    std::string sIp = (g_useLocal) ? "127.0.0.1" : strIP;
    //由于目前网络库不支持IPV6，所以添加配置项，可以选定本地地址
    if (!connection_room_->Create(sIp.c_str(), nPort, 5, 0, nThrdId, 0, GetHelloData(), GetHelloLength())) {
        UWL_ERR("ConnectRoom Faild! IP:%s Port:%d", sIp.c_str(), nPort);
        //assert(false);
        return false;
    }

    return true;
}
bool Robot::ConnectGameWithLock(const std::string& strIP, const int32_t nPort, uint32_t nThrdId) {
    game_connection_->InitKey(KEY_GAMESVR_2_0, ENCRYPT_AES, 0);
    //由于目前网络库不支持IPV6，所以添加配置项，可以选定本地地址
    std::string sIp = (g_useLocal) ? "127.0.0.1" : strIP;
    if (!game_connection_->Create(sIp.c_str(), nPort, 5, 0, nThrdId, 0, GetHelloData(), GetHelloLength())) {
        UWL_ERR("ConnectGame Faild! IP:%s Port:%d", sIp.c_str(), nPort);
        //assert(false);
        return false;
    }

    ////TODO FIXME DECOUPLE WITH MANAGER
    ////@zhuhangmin 20181129 立即加上token信息 以免丢包
    return true;
}
TTueRet Robot::SendRoomRequestWithLock(RequestID nReqId, uint32_t& nDataLen, void *pData, RequestID &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho /*= true*/, uint32_t wait_ms /*= REQ_TIMEOUT_INTERVAL*/) {
    if (!connection_room_) {
        UWL_ERR("m_ConnRoom nil");
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_NOT_EXIST);
    }


    if (!connection_room_->IsConnected()) {
        UWL_ERR("m_ConnRoom not connected");
        //assert(false);
        return std::make_tuple(false, ERR_CONNECT_DISABLE);
    }


    CONTEXT_HEAD	Context = {};
    REQUEST			Request = {};
    REQUEST			Response = {};
    Context.hSocket = connection_room_->GetSocket();
    Context.lSession = 0;
    Context.bNeedEcho = bNeedEcho;
    Request.head.nRepeated = 0;
    Request.head.nRequest = nReqId;
    Request.nDataLen = nDataLen;
    Request.pDataPtr = pData;//////////////

    BOOL bTimeOut = FALSE, bResult = TRUE;
    bResult = connection_room_->SendRequest(&Context, &Request, &Response, bTimeOut, wait_ms);

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

//TTueRet Robot::SendRequestWithLock(RequestID nReqId, uint32_t& nDataLen, void *pData, RequestID &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho /*= true*/, uint32_t wait_ms /*= REQ_TIMEOUT_INTERVAL*/) {
//    if (!connection_game_) {
//        UWL_ERR("m_ConnGame is nil");
//        assert(false);
//        return std::make_tuple(false, ERR_CONNECT_NOT_EXIST);
//    }
//
//
//    if (!connection_game_->IsConnected()) {
//        UWL_ERR("m_ConnGame not connected");
//        assert(false);
//        return std::make_tuple(false, ERR_CONNECT_DISABLE);
//    }
//
//
//    CONTEXT_HEAD	Context = {};
//    REQUEST			Request = {};
//    REQUEST			Response = {};
//    Context.hSocket = connection_game_->GetSocket();
//    Context.lSession = 0;
//    Context.bNeedEcho = bNeedEcho;
//    Request.head.nRepeated = 0;
//    Request.head.nRequest = nReqId;
//    Request.nDataLen = nDataLen;
//    Request.pDataPtr = pData;//////////////
//
//    BOOL bTimeOut = FALSE, bResult = TRUE;
//    bResult = connection_game_->SendRequest(&Context, &Request, &Response, bTimeOut, wait_ms);
//
//    if (!bResult)///if timeout or disconnect 
//    {
//        UWL_ERR("game send quest fail");
//        //assert(false);
//        return std::make_tuple(false, (bTimeOut ? ERR_SENDREQUEST_TIMEOUT : ERR_OPERATE_FAILED));
//    }
//
//
//    nDataLen = Response.nDataLen;
//    nRespId = Response.head.nRequest;
//    pRetData.reset(Response.pDataPtr);
//
//    if (nRespId == GR_ERROR_INFOMATION) {
//        CHAR info[512] = {};
//        sprintf_s(info, "%s", pRetData.get());
//        nDataLen = 0;
//        return std::make_tuple(false, info);
//    }
//    if (0 == nRespId) {
//        //assert(false);
//        UWL_ERR("enter game respId = 0");
//        CHAR info[512] = {};
//        sprintf_s(info, "%s", pRetData.get());
//        nDataLen = 0;
//        return std::make_tuple(false, info);
//    }
//    return std::make_tuple(true, ERR_OPERATE_SUCESS);
//}
//
int Robot::SendRequestWithLock(RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool bNeedEcho /*= true*/) {
    int result = RobotUitls::SendRequest(game_connection_, requestid, val, response, bNeedEcho);
    if (result != kCommSucc) {
        UWL_ERR("game send quest fail");
        //assert(false);
    }
    return result;
}

TTueRet Robot::SendEnterRoom(const ROOM& room, uint32_t nNofifyThrId) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!connection_room_) {
        UWL_ERR("m_ConnRoom nil");
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_NOT_EXIST);
    }

    if (!connection_room_->IsConnected()) {

        if (!ConnectRoomWithLock(room.szGameIP, room.nPort == 0 ? 30629 : room.nPort, nNofifyThrId)) {
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
    UwlGetSockPort(connection_room_->GetSocket(), nClientPort);
    er.dwClientPort = MAKELONG(nClientPort, 0);

    int nServerPort = 0;
    UwlGetPeerPort(connection_room_->GetSocket(), nServerPort);
    er.dwServerPort = MAKELONG(nServerPort, 0);

    UwlGetSockAddr(connection_room_->GetSocket(), er.stClientSockIP);
    UwlGetPeerAddr(connection_room_->GetSocket(), er.stRemoteSockIP);

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

    RequestID nResponse;
    //LPVOID	 pRetData = NULL;
    std::shared_ptr<void> pRetData;
    uint32_t nDataLen = sizeof(ENTER_ROOM);
    auto it = SendRoomRequestWithLock(GR_ENTER_ROOM, nDataLen, &er, nResponse, pRetData, true, 4000);
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
    roomid_ = room.nRoomID;
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

int Robot::SendEnterGame(const ROOM& room, uint32_t nNofifyThrId, std::string sNick, std::string sPortr, int nTableNo, int nChairNo) {
    std::lock_guard<std::mutex> lock(mutex_);
    RequestID nResponse;
    auto pSendData = std::make_unique<BYTE>();
    std::shared_ptr<void> pRetData;
    uint32_t nDataLen = 0;

    //@zhuhangmin 20190218 pb
    TCHAR hard_id[MAX_HARDID_LEN_EX];			// 硬件标识
    xyGetHardID(hard_id);
    game::base::EnterNormalGameReq enter_req;
    enter_req.set_userid(userid_);
    enter_req.set_roomid(roomid_);
    enter_req.set_flag(0);//TODO
    enter_req.set_hardid(hard_id);
    REQUEST response = {};
    auto result = SendRequestWithLock(GR_ENTER_NORMAL_GAME, enter_req, response);
    if (kCommSucc != result) {
        UWL_ERR("ParseFromRequest faild.");
        return kCommFaild;
    }

    game::base::EnterNormalGameResp enter_resp;
    int ret = ParseFromRequest(response, enter_resp);
    if (kCommSucc != ret) {
        UWL_ERR("ParseFromRequest faild.");
        return kCommFaild;
    }

    if (kCommSucc != enter_resp.code()) {
        UWL_ERR("enter game faild. check return[%d]. req = %s", enter_resp.code(), GetStringFromPb(enter_req).c_str());
        return kCommFaild;
    }

    //TODO 银子不够 case
    m_bRunGame = true;

    //@zhuhangmin 20190218 pb
    return kCommSucc;
}

//@zhuhangmin
TTueRet Robot::SendGetNewTable(const ROOM& room, uint32_t nNofifyThrId, NTF_GET_NEWTABLE& lpNewTableInfo) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!connection_room_) {
        UWL_ERR("m_ConnRoom nil");
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_NOT_EXIST);
    }

    if (!connection_room_->IsConnected()) {
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

    RequestID nResponse;
    //LPVOID	 pRetData = NULL;
    std::shared_ptr<void> pRetData;
    uint32_t nDataLen = sizeof(GET_NEWTABLE);
    auto it = SendRoomRequestWithLock(MR_GET_NEWTABLE, nDataLen, &gn, nResponse, pRetData, true, 4000);
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
    std::lock_guard<std::mutex> lock(mutex_);
    if (!connection_room_) {
        UWL_ERR("m_ConnRoom nil");
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_NOT_EXIST);
    }

    if (!connection_room_->IsConnected()) {
        UWL_ERR("room not connected");
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_DISABLE);
    }

    ROOMUSER_PULSE rp = {};
    rp.nUserID = GetUserID();
    rp.nRoomID = GetRoomID();

    RequestID nResponse;
    std::shared_ptr<void> pRetData;
    uint32_t nDataLen = sizeof(ROOMUSER_PULSE);
    return SendRoomRequestWithLock(GR_ROOMUSER_PULSE, nDataLen, &rp, nResponse, pRetData, false);
}
TTueRet	Robot::SendGamePulse() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!game_connection_) {
        UWL_ERR("m_ConnGame is nil");
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_NOT_EXIST);
    }


    if (!game_connection_->IsConnected()) {
        UWL_ERR("m_ConnGame not connected");
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_DISABLE);
    }


    GAME_PULSE gp = {};
    gp.nUserID = GetUserID();
    gp.dwAveDelay = 11;
    gp.dwMaxDelay = 22;

    RequestID nResponse;
    std::shared_ptr<void> pRetData;
    uint32_t nDataLen = sizeof(GAME_PULSE);
    return SendRoomRequestWithLock(GR_GAME_PULSE, nDataLen, &gp, nResponse, pRetData, false);
}

int Robot::GetPlayerRoomStatus() {
    std::lock_guard<std::mutex> lg(mutex_);
    return m_playerRoomStatus;
}

void Robot::SetPlayerRoomStatus(int status) {
    std::lock_guard<std::mutex> lg(mutex_);
    m_playerRoomStatus = status;
    UWL_INF("SetPlayerRoomStatus = %d", status);
}
