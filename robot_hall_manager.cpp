#include "stdafx.h"
#include "robot_hall_manager.h"
#include "robot_utils.h"
#include "setting_manager.h"
#include "Main.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



//外部线程调用方法

int RobotHallManager::Init() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_FUNC("[START ROUTINE]");
    if (kCommSucc != CheckNotInnerThread()) {
        ASSERT_FALSE_RETURN
    }

    auto setting = SettingMgr.GetRobotSettingMap();
    for (auto& kv : setting) {
        auto userid = kv.first;
        logon_status_map_[userid] = HallLogonStatusType::kNotLogon;
    }

    notify_thread_.Initial(std::thread([this] {this->ThreadHallNotify(); }));

    heart_thread_.Initial(std::thread([this] {this->ThreadHallPulse(); }));

    if (kCommSucc != InitDataWithLock()) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int RobotHallManager::Term() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (kCommSucc != CheckNotInnerThread()) {
        ASSERT_FALSE_RETURN
    }

    notify_thread_.Release();
    heart_thread_.Release();
    LOG_FUNC("[EXIT ROUTINE]");
    return kCommSucc;
}


int RobotHallManager::LogonHall(const UserID userid) {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_USERID(userid);
    if (kCommSucc != CheckNotInnerThread()) {
        ASSERT_FALSE_RETURN
    }

    HallLogonStatusType status;
    if (kCommSucc == GetLogonStatusWithLock(userid, status)) {
        if (HallLogonStatusType::kLogon == status) {
            return kCommSucc;
        }
    }

    RobotSetting setting;
    if (kCommSucc != SettingMgr.GetRobotSetting(userid, setting)) {
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
    strcpy_s(logonUser.szPassword, sizeof(logonUser.szPassword), password.c_str());

    RequestID nResponse;
    std::shared_ptr<void> pRetData;
    int nDataLen = sizeof(logonUser);
    auto result = SendHallRequestWithLock(GR_LOGON_USER_V2, nDataLen, &logonUser, nResponse, pRetData);
    if (kCommSucc != result) {
        ASSERT_FALSE;
        if (RobotErrorCode::kOperationFailed == result) {
            LOG_ERROR("hall ResetInitDataWithLock");
            ResetInitDataWithLock();
        }
        return result;
    }

    if (!(nResponse == GR_LOGON_SUCCEEDED || nResponse == GR_LOGON_SUCCEEDED_V2)) {
        LOG_ERROR("userid = %d GR_LOGON_USER_V2 FAIL", userid);
        return kCommFaild;
    }

    SetLogonStatusWithLock(userid, HallLogonStatusType::kLogon);

    LOG_INFO("userid:%d logon hall ok.", userid);
    return kCommSucc;
}

int RobotHallManager::GetHallRoomData(const RoomID& roomid, HallRoomData& hall_room_data) {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_ROOMID(roomid);
    if (kCommSucc != CheckNotInnerThread()) {
        ASSERT_FALSE_RETURN
    }
    return GetHallRoomDataWithLock(roomid, hall_room_data);
}

int RobotHallManager::GetRandomNotLogonUserID(UserID& random_userid) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (kCommSucc != CheckNotInnerThread()) {
        ASSERT_FALSE_RETURN
    }

    HallLogonMap temp_map;
    for (auto& kv : logon_status_map_) {
        UserID userid = kv.first;
        auto status = kv.second;
        if (HallLogonStatusType::kNotLogon == status) {
            temp_map[userid] = status;
        }
    }

    if (temp_map.size() == 0) return kCommFaild;

    //random pick up one
    auto random_pos = 0;
    if (kCommSucc != RobotUtils::GenRandInRange(0, temp_map.size() - 1, random_pos)) {
        return kCommFaild;
    }

    auto random_it = std::next(std::begin(temp_map), random_pos);
    random_userid = random_it->first;
    return kCommSucc;
}

int RobotHallManager::ThreadHallNotify() {
    LOG_INFO("[START ROUTINE] RobotHallManager Notify thread [%d] started", GetCurrentThreadId());
    MSG msg = {};
    while (GetMessage(&msg, 0, 0, 0)) {
        if (UM_DATA_RECEIVED == msg.message) {

            LPCONTEXT_HEAD pContext = (LPCONTEXT_HEAD) (msg.wParam);
            LPREQUEST pRequest = (LPREQUEST) (msg.lParam);
            RequestID requestid = pRequest->head.nRequest;

            OnHallNotify(requestid, pRequest->pDataPtr, pRequest->nDataLen);

            SAFE_DELETE(pContext);
            UwlClearRequest(pRequest);
            SAFE_DELETE(pRequest);
        } else {
            DispatchMessage(&msg);
        }
    }
    LOG_INFO("[EXIT ROUTINE] RobotHallManager Notify thread [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}

int RobotHallManager::OnHallNotify(const RequestID requestid, void* ntf_data_ptr, const int data_size) {
    CHECK_REQUESTID(requestid);
    switch (requestid) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
            OnDisconnHall();
            break;
        default:
            break;
    }
    return kCommSucc;
}

int RobotHallManager::OnDisconnHall() {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_THREAD(notify_thread_);
    LOG_ERROR(_T("与大厅服务断开连接"));
    ResetInitDataWithLock();
    return kCommSucc;
}

int RobotHallManager::SendHallPulse() {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_THREAD(heart_thread_);
    HALLUSER_PULSE hp = {};
    hp.nUserID = 0;
    hp.nAgentGroupID = RobotAgentGroupID;

    RequestID nRespID = 0;
    std::shared_ptr<void> pRetData;
    int nDataLen = sizeof(HALLUSER_PULSE);
    auto result = SendHallRequestWithLock(GR_HALLUSER_PULSE, nDataLen, &hp, nRespID, pRetData, false);
    if (kCommSucc != result) { // @zhuhangmin : chengqi review 抽象接口中不应该调用实例化接口，容易死循环
        ASSERT_FALSE;
        if (RobotErrorCode::kOperationFailed == result) {
            LOG_ERROR("hall ResetInitDataWithLock");
            ResetInitDataWithLock();
        }

        if (RobotErrorCode::kConnectionTimeOut == result) {
            LOG_ERROR("Send hall pulse failed");
            timeout_count_++;
            if (MaxPluseTimeOutCount == timeout_count_) {
                ResetInitDataWithLock();
            }
        }

        return result;
    }

    return kCommSucc;
}

int RobotHallManager::ThreadHallPulse() {
    LOG_INFO("[START ROUTINE] RobotHallManager KeepAlive thread [%d] started", GetCurrentThreadId());
    while (true) {
        DWORD dwRet = WaitForSingleObject(g_hExitServer, PulseInterval);
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }

        if (WAIT_TIMEOUT == dwRet) {

            SendHallPulse();

            SendGetAllRoomData();

        }
    }
    LOG_INFO("[EXIT ROUTINE] RobotHallManager KeepAlive thread [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}

int RobotHallManager::SendGetAllRoomData() {
    std::lock_guard<std::mutex> lock(mutex_);
    SendGetAllRoomDataWithLock();
    return kCommSucc;
}

int RobotHallManager::SendHallRequestWithLock(const RequestID requestid, int& data_size, void *req_data_ptr, RequestID &response_id, std::shared_ptr<void> &resp_data_ptr, bool need_echo /*= true*/) {
    CHECK_REQUESTID(requestid);
    if (!connection_) {
        LOG_ERROR("SendHallRequest m_CoonHall nil ERR_CONNECT_NOT_EXIST nReqId = %d", requestid);
        ASSERT_FALSE_RETURN;
    }

    if (!connection_->IsConnected()) {
        LOG_ERROR("SendHallRequest m_CoonHall not connect ERR_CONNECT_DISABLE nReqId = %d", requestid);
        ASSERT_FALSE_RETURN;
    }

    CONTEXT_HEAD	Context = {};
    REQUEST			Request = {};
    REQUEST			Response = {};
    Context.hSocket = connection_->GetSocket();
    Context.lSession = 0;
    Context.bNeedEcho = need_echo;
    Request.head.nRepeated = 0;
    Request.head.nRequest = requestid;
    Request.nDataLen = data_size;
    Request.pDataPtr = req_data_ptr;

    BOOL timeout = FALSE;
    BOOL result = connection_->SendRequest(&Context, &Request, &Response, timeout, RequestTimeOut);

    if (!result) {
        LOG_ERROR("SendHallRequest m_ConnHall->SendRequest fail bTimeOut = %d, nReqId = %d", timeout, requestid);
        ASSERT_FALSE;

        if (timeout) {
            return RobotErrorCode::kConnectionTimeOut;
        }

        return result;
    }

    data_size = Response.nDataLen;
    response_id = Response.head.nRequest;
    resp_data_ptr.reset(Response.pDataPtr);

    if (response_id == GR_ERROR_INFOMATION) {
        LOG_ERROR("SendHallRequest m_ConnHall->SendRequest fail responseid GR_ERROR_INFOMATION nReqId = %d", requestid);
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int RobotHallManager::InitDataWithLock() {
    if (kCommSucc != ConnectHallWithLock()) {
        LOG_ERROR("ConnectHall failed");
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != SendGetAllRoomDataWithLock()) {
        LOG_ERROR("SendGetAllRoomData failed");
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int RobotHallManager::ConnectHallWithLock() {
    TCHAR szHallSvrIP[MAX_SERVERIP_LEN] = {};
    GetPrivateProfileString(_T("hall_server"), _T("ip"), _T(""), szHallSvrIP, sizeof(szHallSvrIP), g_szIniFile);
    auto nHallSvrPort = GetPrivateProfileInt(_T("hall_server"), _T("port"), 0, g_szIniFile);
    connection_->InitKey(KEY_HALL, ENCRYPT_AES, 0);
    if (!connection_->Create(szHallSvrIP, nHallSvrPort, 5, 0, notify_thread_.GetThreadID(), 0, GetHelloData(), GetHelloLength())) {
        LOG_ERROR("[ROUTE] ConnectHall Faild! IP:%s Port:%d", szHallSvrIP, nHallSvrPort);
        ASSERT_FALSE_RETURN;
    }
    LOG_INFO("ConnectHall OK! IP:%s Port:%d", szHallSvrIP, nHallSvrPort);
    return kCommSucc;
}

int RobotHallManager::SendGetAllRoomDataWithLock() {
    auto setting = SettingMgr.GetRoomSettingMap();
    for (auto& kv : setting) {
        auto roomid = kv.first;
        if (kCommSucc != SendGetRoomDataWithLock(roomid)) {
            LOG_ERROR("cannnot get hall room data = %d", roomid);
            ASSERT_FALSE_RETURN;
        }
    }

    return kCommSucc;
}

int RobotHallManager::SendGetRoomDataWithLock(const RoomID roomid) {
    CHECK_ROOMID(roomid);
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
    if (kCommSucc != result) { // @zhuhangmin : chengqi review 抽象接口中不应该调用实例化接口，容易死循环
        ASSERT_FALSE;
        if (RobotErrorCode::kOperationFailed == result) {
            LOG_ERROR("hall ResetInitDataWithLock");
            ResetInitDataWithLock();
        }
        return result;
    }

    if (nRespID != UR_FETCH_SUCCEEDED) {
        LOG_ERROR("SendHallRequest GR_GET_ROOM fail nRoomId = %d, nResponse = %d", roomid, nRespID);
        ASSERT_FALSE_RETURN;
    }

    SetHallRoomDataWithLock(roomid, (HallRoomData*) pRetData.get());
    return kCommSucc;
}

int RobotHallManager::GetLogonStatusWithLock(const UserID& userid, HallLogonStatusType& status) const {
    CHECK_USERID(userid);
    auto iter = logon_status_map_.find(userid);
    if (iter == logon_status_map_.end()) {
        return kCommFaild;
    }

    status = iter->second;
    return kCommSucc;
}

int RobotHallManager::SetLogonStatusWithLock(const UserID userid, HallLogonStatusType status) {
    CHECK_USERID(userid);
    logon_status_map_[userid] = status;
    return kCommSucc;
}

int RobotHallManager::GetHallRoomDataWithLock(const RoomID& roomid, HallRoomData& hall_room_data) const {
    CHECK_ROOMID(roomid);
    auto iter = room_data_map_.find(roomid);
    if (iter == room_data_map_.end()) {
        return kCommFaild;
    }

    hall_room_data = iter->second;
    return kCommSucc;
}

int RobotHallManager::SetHallRoomDataWithLock(const RoomID roomid, HallRoomData* hall_room_data) {
    CHECK_ROOMID(roomid);
    room_data_map_[roomid] = *hall_room_data;
    return kCommSucc;
}

int RobotHallManager::ResetInitDataWithLock() {
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

int RobotHallManager::ResetDataWithLock() {
    if (connection_) {
        connection_->DestroyEx();
    }

    for (auto& kv : logon_status_map_) {
        auto userid = kv.first;
        SetLogonStatusWithLock(userid, HallLogonStatusType::kNotLogon);
    }
    room_data_map_.clear();
    timeout_count_ = 0;
    return kCommSucc;
}

int RobotHallManager::SnapShotObjectStatus() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("OBJECT ADDRESS [%x]", this);
    LOG_INFO("hall_notify_thread_ [%d]", notify_thread_.GetThreadID());
    LOG_INFO("hall_heart_timer_thread_ [%d]", heart_thread_.GetThreadID());
    LOG_INFO("token [%d]", connection_->GetTokenID());

    LOG_INFO("hall_room_data_map_ size [%d]", room_data_map_.size());
    for (auto& kv : room_data_map_) {
        auto roomid = kv.first;
        auto room = kv.second.room;
        LOG_INFO("roomid [%d] ip [%s] port [%d]", roomid, room.szGameIP, room.nGamePort);
    }

    LOG_INFO("hall_logon_status_map_ size [%d]", logon_status_map_.size());
    auto status_on_count = 0;
    auto status_off_count = 0;
    auto status_unknow_count = 0;
    std::string str = "{";
    for (auto& kv : logon_status_map_) {
        auto userid = kv.first;
        auto status = kv.second;
        if (status == HallLogonStatusType::kLogon) {
            status_on_count++;

            str += "userid ";
            str += "[";
            str += std::to_string(userid);
            str += "] ";
            str += "status ";
            str += "[";
            str += std::to_string((int) status);
            str += "]";
            str += ", ";

        } else if (status == HallLogonStatusType::kNotLogon) {
            status_off_count++;
        } else {
            status_unknow_count++;
        }
    }
    str += "}";

    LOG_INFO("on [%d] off [%d] unknow [%d]", status_on_count, status_off_count, status_unknow_count);
    LOG_INFO("%s", str.c_str());

    return kCommSucc;
}

int RobotHallManager::CheckNotInnerThread() {
    CHECK_NOT_THREAD(notify_thread_);
    CHECK_NOT_THREAD(heart_thread_);
    return kCommSucc;
}