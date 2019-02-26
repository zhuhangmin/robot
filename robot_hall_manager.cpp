#include "stdafx.h"
#include "robot_hall_manager.h"
#include "robot_utils.h"
#include "setting_manager.h"
#include "Main.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

int RobotHallManager::Init() {

    auto setting = SettingMgr.GetRobotSettingMap();
    for (auto& kv : setting) {
        auto userid = kv.first;
        hall_logon_status_map_[userid] = HallLogonStatusType::kNotLogon;
    }

    hall_notify_thread_.Initial(std::thread([this] {this->ThreadHallNotify(); }));

    hall_heart_timer_thread_.Initial(std::thread([this] {this->ThreadHallPluse(); }));

    if (kCommFaild == ConnectHall()) {
        UWL_ERR("ConnectHall() return failed");
        assert(false);
        return kCommFaild;
    }

    SendGetAllRoomData();

    UWL_INF("HallManager::Init Sucessed.");
    return kCommSucc;
}

int RobotHallManager::Term() {
    hall_notify_thread_.Release();
    hall_heart_timer_thread_.Release();
    return kCommSucc;
}

int RobotHallManager::ConnectHall() {
    TCHAR szHallSvrIP[MAX_SERVERIP_LEN] = {};
    GetPrivateProfileString(_T("hall_server"), _T("ip"), _T(""), szHallSvrIP, sizeof(szHallSvrIP), g_szIniFile);
    auto nHallSvrPort = GetPrivateProfileInt(_T("hall_server"), _T("port"), 0, g_szIniFile);

    {
        std::lock_guard<std::mutex> lock(hall_connection_mutex_);
        hall_connection_->InitKey(KEY_HALL, ENCRYPT_AES, 0);

        if (!hall_connection_->Create(szHallSvrIP, nHallSvrPort, 5, 0, hall_notify_thread_.ThreadId(), 0, GetHelloData(), GetHelloLength())) {
            UWL_ERR("[ROUTE] ConnectHall Faild! IP:%s Port:%d", szHallSvrIP, nHallSvrPort);
            assert(false);
            return kCommFaild;
        }
    }
    UWL_INF("ConnectHall OK! IP:%s Port:%d", szHallSvrIP, nHallSvrPort);
    return kCommSucc;
}

int RobotHallManager::SendHallRequestWithLock(const RequestID requestid, int& data_size, void *req_data_ptr, RequestID &response_id, std::shared_ptr<void> &resp_data_ptr, bool need_echo /*= true*/) {
    CHECK_REQUESTID(requestid);
    if (!hall_connection_) {
        UWL_ERR("SendHallRequest m_CoonHall nil ERR_CONNECT_NOT_EXIST nReqId = %d", requestid);
        assert(false);
        return kCommFaild;
    }

    if (!hall_connection_->IsConnected()) {
        UWL_ERR("SendHallRequest m_CoonHall not connect ERR_CONNECT_DISABLE nReqId = %d", requestid);
        assert(false);
        return kCommFaild;
    }

    CONTEXT_HEAD	Context = {};
    REQUEST			Request = {};
    REQUEST			Response = {};
    Context.hSocket = hall_connection_->GetSocket();
    Context.lSession = 0;
    Context.bNeedEcho = need_echo;
    Request.head.nRepeated = 0;
    Request.head.nRequest = requestid;
    Request.nDataLen = data_size;
    Request.pDataPtr = req_data_ptr;

    BOOL bTimeOut = FALSE;
    BOOL bResult = hall_connection_->SendRequest(&Context, &Request, &Response, bTimeOut, RequestTimeOut);

    if (!bResult)///if timeout or disconnect 
    {
        UWL_ERR("SendHallRequest m_ConnHall->SendRequest fail bTimeOut = %d, nReqId = %d", bTimeOut, requestid);
        assert(false);
        return kCommFaild;
        //TODO
        //std::make_tuple(false, (bTimeOut ? ERR_SENDREQUEST_TIMEOUT : ERR_OPERATE_FAILED));
    }

    data_size = Response.nDataLen;
    response_id = Response.head.nRequest;
    resp_data_ptr.reset(Response.pDataPtr);

    if (response_id == GR_ERROR_INFOMATION) {
        UWL_ERR("SendHallRequest m_ConnHall->SendRequest fail responseid GR_ERROR_INFOMATION nReqId = %d", requestid);
        assert(false);
        return kCommFaild;
    }
    return kCommSucc;
}


int RobotHallManager::ThreadHallNotify() {
    UWL_INF(_T("HallNotify thread started. id = %d"), GetCurrentThreadId());

    MSG msg = {};
    while (GetMessage(&msg, 0, 0, 0)) {
        if (UM_DATA_RECEIVED == msg.message) {

            LPCONTEXT_HEAD pContext = (LPCONTEXT_HEAD) (msg.wParam);
            LPREQUEST pRequest = (LPREQUEST) (msg.lParam);

            TokenID	nTokenID = pContext->lTokenID;
            RequestID requestid = pRequest->head.nRequest;

            OnHallNotify(requestid, pRequest->pDataPtr, pRequest->nDataLen);

            SAFE_DELETE(pContext);
            UwlClearRequest(pRequest);
            SAFE_DELETE(pRequest);
        } else {
            DispatchMessage(&msg);
        }
    }
    UWL_INF(_T("HallNotify thread exiting. id = %d"), GetCurrentThreadId());
    return kCommSucc;
}


int RobotHallManager::OnHallNotify(const RequestID requestid, void* ntf_data_ptr, const int data_size) {
    CHECK_REQUESTID(requestid);
    switch (requestid) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
            OnDisconnHall(requestid, ntf_data_ptr, data_size);
            break;
        default:
            break;
    }
    return kCommSucc;
}

int RobotHallManager::OnDisconnHall(const RequestID requestid, void* data_ptr, const int data_size) {
    UWL_ERR(_T("与大厅服务断开连接"));
    std::lock_guard<std::mutex> lock(hall_connection_mutex_);
    hall_connection_->DestroyEx();
    for (auto& kv : hall_logon_status_map_) {
        auto userid = kv.first;
        SetLogonStatusWithLock(userid, HallLogonStatusType::kNotLogon);
    }

    return kCommSucc;
}

int RobotHallManager::ThreadHallPluse() {
    UWL_INF(_T("Hall KeepAlive thread started. id = %d"), GetCurrentThreadId());
    while (true) {
        DWORD dwRet = WaitForSingleObject(g_hExitServer, PluseInterval);
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }

        if (WAIT_TIMEOUT == dwRet) {

            SendHallPluse();

            SendGetAllRoomData();

        }
    }
    UWL_INF(_T("Hall KeepAlive thread exiting. id = %d"), GetCurrentThreadId());
    return kCommSucc;
}


int RobotHallManager::LogonHall(const UserID userid) {
    CHECK_USERID(userid);
    std::lock_guard<std::mutex> lock(hall_connection_mutex_);

    RobotSetting setting;
    if (kCommFaild == SettingMgr.GetRobotSetting(userid, setting)) {
        return kCommFaild;
    }
    std::string password = setting.password;

    //账号对应robot是否已经生成, 是否已经登入大厅
    LOGON_USER_V2  logonUser = {};
    logonUser.nUserID = userid;
    xyGetHardID(logonUser.szHardID);  // 硬件ID
    xyGetVolumeID(logonUser.szVolumeID);
    xyGetMachineID(logonUser.szMachineID);
    UwlGetSystemVersion(logonUser.dwSysVer);
    logonUser.nAgentGroupID = RobotAgentGroupID; // 使用888作为组号
    logonUser.dwLogonFlags |= (FLAG_LOGON_INTER | FLAG_LOGON_ROBOT_TOOL);
    logonUser.nLogonSvrID = 0;
    logonUser.nHallBuildNO = 20160414;
    logonUser.nHallNetDelay = 1;
    logonUser.nHallRunCount = 1;
    strcpy_s(logonUser.szPassword, password.c_str());

    RequestID nResponse;
    std::shared_ptr<void> pRetData;
    int nDataLen = sizeof(logonUser);
    if (kCommFaild == SendHallRequestWithLock(GR_LOGON_USER_V2, nDataLen, &logonUser, nResponse, pRetData))
        return kCommFaild;

    if (!(nResponse == GR_LOGON_SUCCEEDED || nResponse == GR_LOGON_SUCCEEDED_V2)) {
        UWL_ERR("userid = %d GR_LOGON_USER_V2 FAIL", userid);
        return kCommFaild;
    }

    SetLogonStatusWithLock(userid, HallLogonStatusType::kLogon);

    UWL_INF("userid:%d logon hall ok.", userid);
    return kCommSucc;
}

int RobotHallManager::SendHallPluse() {
    std::lock_guard<std::mutex> lock(hall_connection_mutex_);
    HALLUSER_PULSE hp = {};
    hp.nUserID = 0;
    hp.nAgentGroupID = RobotAgentGroupID;

    RequestID nRespID = 0;
    std::shared_ptr<void> pRetData;
    int nDataLen = sizeof(HALLUSER_PULSE);
    auto result = SendHallRequestWithLock(GR_HALLUSER_PULSE, nDataLen, &hp, nRespID, pRetData, false);
    if (result == kCommFaild) {
        UWL_ERR("Send hall pluse failed");
        return kCommSucc;
    }
    return kCommFaild;
}

int RobotHallManager::SendGetAllRoomData() {
    auto setting = SettingMgr.GetRoomSettingMap();
    for (auto& kv : setting) {
        auto roomid = kv.first;
        if (kCommFaild == SendGetRoomData(roomid)) {
            assert(false);
            UWL_ERR("cannnot get hall room data = %d", roomid);
        }
    }

    return kCommFaild;
}

int RobotHallManager::SendGetRoomData(const RoomID roomid) {
    CHECK_ROOMID(roomid);
    /*if (kCommSucc != RobotUtils::IsValidRoomID(roomid)) {
        assert(false); return kCommFaild;
        }
        return kCommSucc;*/
    std::lock_guard<std::mutex> lock(hall_connection_mutex_);
    GameID game_id = SettingMgr.GetGameID();
    CHECK_GAMEID(game_id);
    GET_ROOM  gr = {};
    gr.nGameID = game_id;
    gr.nRoomID = roomid;
    gr.nAgentGroupID = RobotAgentGroupID;
    gr.dwFlags |= FLAG_GETROOMS_INCLUDE_HIDE;
    gr.dwFlags |= FLAG_GETROOMS_INCLUDE_ONLINES;

    RequestID nRespID = 0;
    std::shared_ptr<void> pRetData;
    int nDataLen = sizeof(GET_ROOM);
    auto result = SendHallRequestWithLock(GR_GET_ROOM, nDataLen, &gr, nRespID, pRetData);
    if (result == kCommFaild) {
        UWL_ERR("Send hall pluse failed");
    }

    if (nRespID != UR_FETCH_SUCCEEDED) {
        UWL_ERR("SendHallRequest GR_GET_ROOM fail nRoomId = %d, nResponse = %d", roomid, nRespID);
        assert(false);
        return kCommFaild;
    }

    SetHallRoomDataWithLock(roomid, (HallRoomData*) pRetData.get());
    return kCommSucc;
}

int RobotHallManager::GetLogonStatusWithLock(const UserID& userid, HallLogonStatusType& status) const {
    CHECK_USERID(userid);
    auto& iter = hall_logon_status_map_.find(userid);
    if (iter == hall_logon_status_map_.end()) {
        return kCommFaild;
    }

    status = iter->second;
    return kCommSucc;
}

int RobotHallManager::SetLogonStatusWithLock(const UserID userid, HallLogonStatusType status) {
    CHECK_USERID(userid);
    hall_logon_status_map_[userid] = status;
    return kCommSucc;
}

int RobotHallManager::GetHallRoomData(const RoomID& roomid, HallRoomData& hall_room_data) {
    CHECK_ROOMID(roomid);
    std::lock_guard<std::mutex> lock(hall_connection_mutex_);
    return GetHallRoomDataWithLock(roomid, hall_room_data);
}

int RobotHallManager::GetHallRoomDataWithLock(const RoomID& roomid, HallRoomData& hall_room_data) const {
    CHECK_ROOMID(roomid);
    auto& iter = hall_room_data_map_.find(roomid);
    if (iter == hall_room_data_map_.end()) {
        return kCommFaild;
    }

    hall_room_data = iter->second;
    return kCommSucc;
}

int RobotHallManager::SetHallRoomDataWithLock(const RoomID roomid, HallRoomData* hall_room_data) {
    CHECK_ROOMID(roomid);
    hall_room_data_map_[roomid] = *hall_room_data;
    return kCommSucc;
}

int RobotHallManager::GetRandomNotLogonUserID(UserID& random_userid) const {
    //filter robots who are not logon on hall
    std::lock_guard<std::mutex> lock(hall_connection_mutex_);
    HallLogonMap temp_map;
    for (auto& kv : hall_logon_status_map_) {
        UserID userid = kv.first;
        auto status = kv.second;
        if (HallLogonStatusType::kNotLogon == status) {
            temp_map[userid] = status;
        }
    }

    if (temp_map.size() == 0) return kCommFaild;

    //random pick up one
    auto random_pos = 0;
    if (kCommFaild == RobotUtils::GenRandInRange(0, temp_map.size() - 1, random_pos)) {
        return kCommFaild;
    }

    auto random_it = std::next(std::begin(temp_map), random_pos);
    random_userid = random_it->first;
    return kCommSucc;
}
