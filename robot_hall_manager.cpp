#include "stdafx.h"
#include "robot_hall_manager.h"
#include "robot_utils.h"
#include "setting_manager.h"
#include "main.h"
#include "robot_statistic.h"
#include "PBReq.h"

//外部线程调用方法

int RobotHallManager::Init() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO_FUNC("\t[START]");
    if (kCommSucc != CheckNotInnerThread()) {
        ASSERT_FALSE_RETURN
    }

    auto setting = SettingMgr.GetRobotSettingMap();
    for (auto& kv : setting) {
        auto userid = kv.first;
        logon_status_map_[userid] = HallLogonStatusType::kNotLogon;
    }

    notify_thread_.Initial(std::thread([this] {this->ThreadNotify(); }));

    heart_thread_.Initial(std::thread([this] {this->ThreadTimer(); }));

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
    LOG_INFO_FUNC("[EXIT]");
    return kCommSucc;
}


int RobotHallManager::LogonHall(const UserID& userid) {
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
        ASSERT_FALSE_RETURN;
    }
    const auto& password = setting.password;

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
    const auto result = SendRequestWithLock(GR_LOGON_USER_V2, nDataLen, &logonUser, nResponse, pRetData);
    if (kCommSucc != result) {
        ASSERT_FALSE;
        if (kOperationFailed == result) {
            LOG_ERROR("hall ResetDataWithLock");
            ResetDataWithLock();
        }
        return result;
    }

    if (!(nResponse == GR_LOGON_SUCCEEDED || nResponse == GR_LOGON_SUCCEEDED_V2)) {
        LOG_ERROR("userid  = [%d] GR_LOGON_USER_V2 FAIL", userid);
        ASSERT_FALSE_RETURN;
    }

    SetLogonStatusWithLock(userid, HallLogonStatusType::kLogon);
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
        const auto userid = kv.first;
        const auto status = kv.second;
        if (HallLogonStatusType::kNotLogon == status) {
            temp_map[userid] = status;
        }
    }

    if (temp_map.empty()) ASSERT_FALSE_RETURN;

    //random pick up one
    auto random_pos = 0;
    if (kCommSucc != RobotUtils::GenRandInRange(0, temp_map.size() - 1, random_pos)) {
        ASSERT_FALSE_RETURN;
    }

    const auto random_it = std::next(std::begin(temp_map), random_pos);
    random_userid = random_it->first;
    return kCommSucc;
}

int RobotHallManager::SetLogonStatus(const UserID& userid, const HallLogonStatusType& status) {
    std::lock_guard<std::mutex> lock(mutex_);
    return SetLogonStatusWithLock(userid, status);
}

int RobotHallManager::ThreadNotify() {
    LOG_INFO("\t[START] notify thread [%d] started", GetCurrentThreadId());
    MSG msg = {};
    while (GetMessage(&msg, 0, 0, 0)) {
        if (UM_DATA_RECEIVED == msg.message) {
            auto pContext = reinterpret_cast<LPCONTEXT_HEAD>(msg.wParam);
            auto pRequest = reinterpret_cast<LPREQUEST>(msg.lParam);
            const auto requestid = pRequest->head.nRequest;

            OnHallNotify(requestid, pRequest->pDataPtr, pRequest->nDataLen);

            SAFE_DELETE(pContext);
            UwlClearRequest(pRequest);
            SAFE_DELETE(pRequest);
        } else {
            DispatchMessage(&msg);
        }
    }
    LOG_INFO("[EXIT] notify thread [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}

int RobotHallManager::OnHallNotify(const RequestID& requestid, void* ntf_data_ptr, const int& data_size) {
    CHECK_REQUESTID(requestid);
    if (requestid != PB_NOTIFY_TO_CLIENT) {
        LOG_INFO("HALL [RECV] requestid [%d] [%s]", requestid, REQ_STR(requestid));
    }

    int result = kCommSucc;
    switch (requestid) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
            result = OnDisconnect();
            break;
        default:
            break;
    }
    EVENT_TRACK(EventType::kRecv, requestid);

    if (kCommSucc != result) {
        LOG_ERROR("requestid  = [%d], result  = [%d]", requestid, result);
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int RobotHallManager::OnDisconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_THREAD(notify_thread_);
    LOG_WARN("OnDisconnect Hall");
    return kCommSucc;
}

int RobotHallManager::SendHallPulse() {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_THREAD(heart_thread_);
    HALLUSER_PULSE hp;
    hp.nUserID = 0;
    hp.nAgentGroupID = RobotAgentGroupID;

    RequestID nRespID = 0;
    std::shared_ptr<void> pRetData;
    int nDataLen = sizeof(HALLUSER_PULSE);
    const auto result = SendRequestWithLock(GR_HALLUSER_PULSE, nDataLen, &hp, nRespID, pRetData, false); //大厅心跳 不用回包 false
    if (kCommSucc != result) { // @zhuhangmin : chengqi review 抽象接口中不应该调用实例化接口，容易死循环
        ASSERT_FALSE;
        if (kOperationFailed == result) {
            LOG_ERROR("hall InitDataWithLock");
            ResetDataWithLock();
        }

        if (kConnectionTimeOut == result) {
            LOG_ERROR("Send hall pulse failed");
            timeout_count_++;
            if (MaxPluseTimeOutCount == timeout_count_) {
                ResetDataWithLock();
            }
        }

        return result;
    }

    return kCommSucc;
}

int RobotHallManager::ThreadTimer() {
    LOG_INFO("\t[START] timer thread [%d] started", GetCurrentThreadId());
    while (true) {
        const auto dwRet = WaitForSingleObject(g_hExitServer, HallPulseInterval);
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }

        if (WAIT_TIMEOUT == dwRet) {
            if (kCommSucc != KeepConnection()) continue;

            SendHallPulse();

            SendGetAllRoomData();
        }
    }
    LOG_INFO("[EXIT] timer thread [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}

int RobotHallManager::SendGetAllRoomData() {
    std::lock_guard<std::mutex> lock(mutex_);
    SendGetAllRoomDataWithLock();
    return kCommSucc;
}

int RobotHallManager::SendRequestWithLock(const RequestID& requestid, int& data_size, void *req_data_ptr, RequestID &response_id, std::shared_ptr<void> &resp_data_ptr, const bool& need_echo /*= true*/) const {
    CHECK_REQUESTID(requestid);
    if (!connection_) {
        LOG_ERROR("connection not exist");
        return kCommFaild;
    }

    if (!connection_->IsConnected()) {
        LOG_ERROR("not connected");
        return kCommFaild;
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

    EVENT_TRACK(EventType::kSend, requestid);
    BOOL timeout = FALSE;
    BOOL result = connection_->SendRequest(&Context, &Request, &Response, timeout, RequestTimeOut);
    EVENT_TRACK(EventType::kRecv, requestid);

    if (!result) {
        LOG_ERROR("SendHallRequest m_ConnHall->SendRequest fail bTimeOut  = [%d], nReqId  = [%d]", timeout, requestid);
        if (timeout) {
            EVENT_TRACK(EventType::kErr, kConnectionTimeOut);
            ASSERT_FALSE;
            return kConnectionTimeOut;
        }

        EVENT_TRACK(EventType::kErr, kOperationFailed);
        ASSERT_FALSE;
        return result;
    }

    if (requestid != GR_HALLUSER_PULSE &&
        requestid != GR_LOGON_USER_V2 &&
        requestid != GR_GET_ROOM) {
        LOG_INFO("connection [%x] [SEND] requestid [%d] [%s]", connection_.get(), requestid, REQ_STR(requestid));
    }

    data_size = Response.nDataLen;
    response_id = Response.head.nRequest;
    resp_data_ptr.reset(Response.pDataPtr);

    if (response_id == GR_ERROR_INFOMATION) {
        LOG_ERROR("SendHallRequest m_ConnHall->SendRequest fail responseid GR_ERROR_INFOMATION nReqId  = [%d]", requestid);
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int RobotHallManager::InitDataWithLock() {
    CHECK_CONNECTION(connection_);
    if (kCommSucc != ConnectWithLock()) {
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != SendGetAllRoomDataWithLock()) {
        LOG_ERROR("SendGetAllRoomData failed");
        ASSERT_FALSE_RETURN;
    }
    need_reconnect_ = false;
    return kCommSucc;
}

int RobotHallManager::ConnectWithLock() {
    CHECK_CONNECTION(connection_);
    TCHAR szHallSvrIP[MAX_SERVERIP_LEN] = {};
    GetPrivateProfileString(_T("hall_server"), _T("ip"), _T(""), szHallSvrIP, sizeof szHallSvrIP, g_szIniFile);
    auto nHallSvrPort = GetPrivateProfileInt(_T("hall_server"), _T("port"), 0, g_szIniFile);
    LOG_INFO("\t[START] try connect hall ... ip [%s] port [%d]", szHallSvrIP, nHallSvrPort);
    connection_->InitKey(KEY_HALL, ENCRYPT_AES, 0);
    if (!connection_->Create(szHallSvrIP, nHallSvrPort, 5, 0, notify_thread_.GetThreadID(), 0, GetHelloData(), GetHelloLength())) {
        LOG_ERROR("\t[START] connect hall faild! ip [%s] port [%d]", szHallSvrIP, nHallSvrPort);
        EVENT_TRACK(EventType::kErr, kCreateHallConnFailed);
        ASSERT_FALSE_RETURN;
    }
    LOG_INFO("\t[START] hall connect ok! token [%d] ip [%s] port [%d]", connection_->GetTokenID(), szHallSvrIP, nHallSvrPort);
    return kCommSucc;
}

int RobotHallManager::SendGetAllRoomDataWithLock() {
    auto setting = SettingMgr.GetRoomSettingMap();
    for (auto& kv : setting) {
        const auto roomid = kv.first;
        if (kCommSucc != SendGetRoomDataWithLock(roomid)) {
            LOG_ERROR("cannnot get hall room data  = [%d]", roomid);
            ASSERT_FALSE;
            continue;
        }
    }

    return kCommSucc;
}

int RobotHallManager::SendGetRoomDataWithLock(const RoomID& roomid) {
    CHECK_ROOMID(roomid);
    const auto game_id = SettingMgr.GetGameID();
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
    const auto result = SendRequestWithLock(GR_GET_ROOM, nDataLen, &gr, nRespID, pRetData);
    if (kCommSucc != result) { // @zhuhangmin : chengqi review 抽象接口中不应该调用实例化接口，容易死循环
        ASSERT_FALSE;
        if (kOperationFailed == result) {
            LOG_ERROR("hall InitDataWithLock");
            ResetDataWithLock();
        }
        return result;
    }

    if (nRespID != UR_FETCH_SUCCEEDED) {
        LOG_ERROR("SendHallRequest GR_GET_ROOM fail nRoomId  = [%d], nResponse  = [%d]", roomid, nRespID);
        ASSERT_FALSE_RETURN;
    }

    SetHallRoomDataWithLock(roomid, static_cast<HallRoomData*>(pRetData.get()));
    return kCommSucc;
}

int RobotHallManager::GetLogonStatusWithLock(const UserID& userid, HallLogonStatusType& status) const {
    CHECK_USERID(userid);
    const auto& iter = logon_status_map_.find(userid);
    if (iter == logon_status_map_.end()) {
        return kCommFaild;
    }

    status = iter->second;
    return kCommSucc;
}

int RobotHallManager::SetLogonStatusWithLock(const UserID& userid, const HallLogonStatusType& status) {
    CHECK_USERID(userid);
    logon_status_map_[userid] = status;
    return kCommSucc;
}

int RobotHallManager::GetHallRoomDataWithLock(const RoomID& roomid, HallRoomData& hall_room_data) const {
    CHECK_ROOMID(roomid);
    const auto& iter = room_data_map_.find(roomid);
    if (iter == room_data_map_.end()) {
        ASSERT_FALSE_RETURN;
    }

    hall_room_data = iter->second;
    return kCommSucc;
}

int RobotHallManager::SetHallRoomDataWithLock(const RoomID& roomid, HallRoomData* hall_room_data) {
    CHECK_ROOMID(roomid);
    room_data_map_[roomid] = *hall_room_data;
    return kCommSucc;
}

int RobotHallManager::ResetDataWithLock() {
    if (connection_) {
        connection_->DestroyEx();
    }

    for (auto& kv : logon_status_map_) {
        const auto userid = kv.first;
        SetLogonStatusWithLock(userid, HallLogonStatusType::kNotLogon);
    }
    room_data_map_.clear();
    timeout_count_ = 0;
    need_reconnect_ = true;
    return kCommFaild;
}

int RobotHallManager::KeepConnection() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (need_reconnect_) {
        return ResetDataWithLock();
    }
    return kCommSucc;
}

int RobotHallManager::SnapShotObjectStatus() {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_CONNECTION(connection_);
    TCHAR szHallSvrIP[MAX_SERVERIP_LEN] = {};
    GetPrivateProfileString(_T("hall_server"), _T("ip"), _T(""), szHallSvrIP, sizeof szHallSvrIP, g_szIniFile);
    auto nHallSvrPort = GetPrivateProfileInt(_T("hall_server"), _T("port"), 0, g_szIniFile);
    LOG_INFO("connection = %x", connection_.get());
    LOG_INFO("hall ip [%s]", szHallSvrIP);
    LOG_INFO("hall port [%d]", nHallSvrPort);
    LOG_INFO("hall_notify_thread_ [%d]", notify_thread_.GetThreadID());
    LOG_INFO("hall_heart_timer_thread_ [%d]", heart_thread_.GetThreadID());
    LOG_INFO("token [%d]", connection_->GetTokenID());

    LOG_INFO("hall_room_data_map_ size [%d]", room_data_map_.size());
    for (auto& kv : room_data_map_) {
        const auto roomid = kv.first;
        const auto room = kv.second.room;
        LOG_INFO("roomid [%d] ip [%s] port [%d]", roomid, room.szGameIP, room.nGamePort);
    }

    LOG_INFO("hall_logon_status_map_ size [%d]", logon_status_map_.size());
    auto status_on_count = 0;
    auto status_off_count = 0;
    auto status_unknow_count = 0;
    std::string str = "{";
    for (auto& kv : logon_status_map_) {
        const auto userid = kv.first;
        const auto status = kv.second;
        if (status == HallLogonStatusType::kLogon) {
            status_on_count++;

            str += "userid ";
            str += "[";
            str += std::to_string(userid);
            str += "] ";
            str += "status ";
            str += "[";
            str += std::to_string(static_cast<int>(status));
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
    LOG_INFO(" [%s]", str.c_str());

    return kCommSucc;
}

int RobotHallManager::BriefInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("hall_logon_status_map_ size [%d]", logon_status_map_.size());
    auto status_on_count = 0;
    auto status_off_count = 0;
    auto status_unknow_count = 0;
    std::string str = "{";
    for (auto& kv : logon_status_map_) {
        const auto userid = kv.first;
        const auto status = kv.second;
        if (status == HallLogonStatusType::kLogon) {
            status_on_count++;

            str += "userid ";
            str += "[";
            str += std::to_string(userid);
            str += "] ";
            str += "status ";
            str += "[";
            str += std::to_string(static_cast<int>(status));
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
    LOG_INFO(" [%s]", str.c_str());
    return kCommSucc;
}


int RobotHallManager::CheckNotInnerThread() {
    CHECK_NOT_THREAD(notify_thread_);
    CHECK_NOT_THREAD(heart_thread_);
    return kCommSucc;
}