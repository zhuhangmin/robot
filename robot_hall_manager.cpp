#include "stdafx.h"
#include "robot_hall_manager.h"
#include "robot_utils.h"
#include "setting_manager.h"
#include "main.h"
#include "robot_statistic.h"
#include "PBReq.h"
#include "deposit_data_manager.h"

//外部线程调用方法

int RobotHallManager::Init() {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO_FUNC("\t[START]");

    auto setting = SettingConfig.GetRobotSettingMap();
    for (auto& kv : setting) {
        auto userid = kv.first;
        logon_status_map_[userid] = HallLogonStatusType::kNotLogon;
    }

    notify_thread_.Initial(std::thread([this] {this->ThreadNotify(); }));

    timer_thread_.Initial(std::thread([this] {this->ThreadTimer(); }));

    if (kCommSucc != InitDataWithLock()) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int RobotHallManager::Term() {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);

    notify_thread_.Release();
    timer_thread_.Release();
    logon_status_map_.clear();
    room_data_map_.clear();
    update_depost_map_.clear();
    timeout_count_ = 0;
    need_reconnect_ = false;
    timestamp_ = 0;
    room_data_timestamp_ = 0;
    if (connection_) {
        LOG_WARN("token [%d] [%s] DestroyEx", connection_->GetTokenID(), __FUNCTION__);
        connection_->DestroyEx();
    }

    LOG_INFO_FUNC("[EXIT]");
    return kCommSucc;
}


int RobotHallManager::LogonHall(const UserID& userid) {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_USERID(userid);

    HallLogonStatusType status;
    if (kCommSucc == GetLogonStatusWithLock(userid, status)) {
        if (HallLogonStatusType::kLogon == status) {
            return kCommSucc;
        }
    }

    if (kCommSucc != LogonHallWithLock(userid)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int RobotHallManager::SetDepositUpdate(const UserID& userid) {
    // 允许deposit_http_manager调用。
    // 注意RobotHallManager内不要使用DepositHttpManager容易嵌套死锁
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_USERID(userid);
    update_depost_map_[userid] = userid;
    return kCommSucc;
}

int RobotHallManager::GetUserGameInfo(const UserID& userid) {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_USERID(userid);
    if (kCommSucc !=GetUserGameInfoWithLock(userid)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int RobotHallManager::GetHallRoomData(const RoomID& roomid, HallRoomData& hall_room_data) const {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_ROOMID(roomid);
    return GetHallRoomDataWithLock(roomid, hall_room_data);
}

int RobotHallManager::GetRandomNotLogonUserID(UserID& random_userid) {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);

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
    int64_t random_pos = 0;
    if (kCommSucc != RobotUtils::GenRandInRange(0, temp_map.size() - 1, random_pos)) {
        ASSERT_FALSE_RETURN;
    }

    const auto random_it = std::next(std::begin(temp_map), random_pos);
    random_userid = random_it->first;
    return kCommSucc;
}

// 银子更新队列中 COPY 允许脏读
RobotUserIDMap RobotHallManager::GetUpdateDepositMap() const {
    return update_depost_map_;
}

int RobotHallManager::GetGamePort() const {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    auto room_setting_map = SettingConfig.GetRoomSettingMap();
    if (room_setting_map.empty()) {
        LOG_ERROR(_T("room_setting_map empty"));
        ASSERT_FALSE_RETURN;
    }

    auto game_port = InvallidPort;
    for (auto& kv: room_setting_map) {
        const auto roomid = kv.first;
        HallRoomData hall_room_data = {};
        if (kCommSucc != GetHallRoomDataWithLock(roomid, hall_room_data)) {
            LOG_ERROR("GetHallRoomData room id  = [%d] failed", roomid);
            ASSERT_FALSE_RETURN;
        }
        const auto port = hall_room_data.room.nGamePort;
        LOG_INFO("[CHECK GAME PORT] roomid [%d] game port [%d]", roomid, port);
        if (InvalidPort == port) continue;

        if (game_port == InvallidPort) {
            game_port = port; // 第一个房间
        } else {
            if (game_port != port) { // 第二个房间 检查是否所有房间 game port 一致
                LOG_WARN("[CHECK GAME PORT] roomid [%d] game port [%d] diff with later one [%d], all room should have the same game port!", roomid, game_port, port);
                ASSERT_FALSE_RETURN;
            }
        }
    }

    return game_port;
}

int RobotHallManager::SetLogonStatus(const UserID& userid, const HallLogonStatusType& status) {
    CHECK_MAIN_OR_LAUNCH_THREAD();
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

            OnNotify(requestid, pRequest->pDataPtr, pRequest->nDataLen);

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

int RobotHallManager::OnNotify(const RequestID& requestid, void* ntf_data_ptr, const int& data_size) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        CHECK_REQUESTID(requestid);
        if (requestid != PB_NOTIFY_TO_CLIENT) {
            if (connection_) {
                LOG_INFO("HALL token [%d] [RECV] requestid [%d] [%s]", connection_->GetTokenID(), requestid, REQ_STR(requestid));
            }
        }
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
    ResetDataWithLock();
    LOG_WARN("OnDisconnect Game Info");
    return kCommSucc;
}

int RobotHallManager::SendPulse() {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_THREAD(timer_thread_);
    const auto now = std::time(nullptr);
    if (now - timestamp_ > HallPulseInterval) {
        timestamp_ = now;
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
    }

    return kCommSucc;
}

int RobotHallManager::ThreadTimer() {
    LOG_INFO("\t[START] timer thread [%d] started", GetCurrentThreadId());
    while (true) {
        const auto dwRet = WaitForSingleObject(g_hExitServer, TimerInterval);
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }

        if (WAIT_TIMEOUT == dwRet) {
            if (!g_inited) continue;
            if (kCommSucc != KeepConnection()) continue;
            CHECK_THREAD(timer_thread_);

            SendPulse();

            SendUpdateUserDeposit();
        }
    }
    LOG_INFO("[EXIT] timer thread [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}

int RobotHallManager::SendUpdateUserDeposit() {
    RobotUserIDMap copy_map;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        copy_map = update_depost_map_;
    }
    for (const auto& kv : copy_map) {
#ifdef CURRENT_DELAY
        //"正式版防止大量消息冲击后端服务器 [副作用] 正式版机器人启动较慢"
        SLEEP_FOR(CurrentDealy);
#endif
        const auto userid = kv.first;
        //所有机器人大厅连接共用一个，防止某些机器人不在setting不发心跳
        //每次刷新硬件码
        if (kCommSucc != LogonHallWithLock(userid)) {
            {// 操作失败 需要从队列中移除
                std::lock_guard<std::mutex> lock(mutex_);
                update_depost_map_.erase(userid);
            }
            continue;
        }

        if (kCommSucc != GetUserGameInfoWithLock(userid)) {
            {// 操作失败 需要从队列中移除
                std::lock_guard<std::mutex> lock(mutex_);
                update_depost_map_.erase(userid);
            }
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            update_depost_map_.erase(userid);
        }
    }
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

    if (requestid != GR_LOGON_USER_V2 &&
        requestid != GR_HALLUSER_PULSE &&
        requestid != GR_GET_ROOM &&
        requestid != MR_QUERY_USER_GAMEINFO) {
        LOG_INFO("hall [SEND] requestid [%d] [%s]", requestid, REQ_STR(requestid));
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

int RobotHallManager::ConnectWithLock() const {
    CHECK_CONNECTION(connection_);
    TCHAR szHallSvrIP[MAX_SERVERIP_LEN] = {};
    GetPrivateProfileString(_T("hall_server"), _T("ip"), _T(""), szHallSvrIP, sizeof szHallSvrIP, g_szIniFile);
    const auto nHallSvrPort = GetPrivateProfileInt(_T("hall_server"), _T("port"), 0, g_szIniFile);
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
    auto setting = SettingConfig.GetRoomSettingMap();
    for (auto& kv : setting) {
        const auto roomid = kv.first;
        if (kCommSucc != SendGetRoomDataWithLock(roomid)) {
            LOG_ERROR("cannnot get hall room data  = [%d]", roomid);
            ASSERT_FALSE;
        }
    }

    return kCommSucc;
}

int RobotHallManager::SendGetRoomDataWithLock(const RoomID& roomid) {
    CHECK_ROOMID(roomid);
    const auto game_id = SettingConfig.GetGameID();
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

int RobotHallManager::GetUserGameInfoWithLock(const UserID& userid) {
    // 只拉去机器人列表中的
    auto exist = false;
    if (kCommSucc != SettingConfig.IsRobotSettingExist(userid, exist)) {
        ASSERT_FALSE_RETURN;
    }
    if (!exist) ASSERT_FALSE_RETURN;

    QUERY_USER_GAMEINFO query = {};
    query.nUserID = userid;
    query.nGameID = SettingConfig.GetGameID();
    query.dwQueryFlags = FLAG_QUERYUSERGAMEINFO_HANDPHONE;
    UWL_ADDR stIPAddr;
    if (!UwlGetSockAddr(connection_->GetSocket(), stIPAddr)) {
        ASSERT_FALSE_RETURN;
    }
    xyGetHardID(query.szHardID);

    RequestID nRespID = 0;
    std::shared_ptr<void> pRetData;
    int nDataLen = sizeof query;
    const auto result = SendRequestWithLock(MR_QUERY_USER_GAMEINFO, nDataLen, &query, nRespID, pRetData);
    if (kCommSucc != result) {
        ASSERT_FALSE;
        if (kOperationFailed == result) {
            LOG_ERROR("hall ResetDataWithLock");
            ResetDataWithLock();
        }
        return result;
    }

    if (nRespID != UR_OPERATE_SUCCEEDED) {
        LOG_ERROR("SendHallRequest GetUserGameInfo fail userid [%d], nResponse [%d] [%s]", userid, nRespID, ERR_STR(nRespID));
        ASSERT_FALSE_RETURN;
    }

    const auto& game_info = static_cast<USER_GAMEINFO_MB*>(pRetData.get());
    DepositDataMgr.SetDeposit(userid, game_info->nDeposit);
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
        LOG_WARN("token [%d] [%s] DestroyEx", connection_->GetTokenID(), __FUNCTION__);
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
        return InitDataWithLock();
    }
    return kCommSucc;
}

bool RobotHallManager::IsInDepositUpdateProcess(const UserID& userid) {
    if (update_depost_map_.find(userid) != update_depost_map_.end()) {
        return true;
    }
    return false;
}

int RobotHallManager::LogonHallWithLock(const UserID& userid) {

    const auto setting = SettingConfig.GetRobotSetting(userid);
    if (setting.userid == InvalidUserID) ASSERT_FALSE_RETURN;
    const auto& password = setting.password;
    const auto& nickname = setting.nickname;


    //账号对应robot是否已经生成, 是否已经登入大厅
    LOGON_USER_V2  logonUser = {};
    logonUser.nUserID = userid;
    xyGetHardID(logonUser.szHardID);  // 硬件ID
    xyGetVolumeID(logonUser.szVolumeID);
    xyGetMachineID(logonUser.szMachineID);
    UwlGetSystemVersion(logonUser.dwSysVer);
    logonUser.nAgentGroupID = RobotAgentGroupID; // 使用888作为组号
    logonUser.dwLogonFlags |= FLAG_LOGON_INTER | FLAG_LOGON_ROBOT_TOOL;
    logonUser.nLogonSvrID = 0;
    logonUser.nHallBuildNO = 20160414;
    logonUser.nHallNetDelay = 1;
    logonUser.nHallRunCount = 1;
    strcpy_s(logonUser.szPassword, sizeof logonUser.szPassword, password.c_str());
    strcpy_s(logonUser.szUsername, sizeof logonUser.szUsername, nickname.c_str());

    RequestID nResponse;
    std::shared_ptr<void> pRetData;
    int nDataLen = sizeof logonUser;
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

int RobotHallManager::SnapShotObjectStatus() {
    CHECK_MAIN_OR_LAUNCH_THREAD();
#ifdef _DEBUG
    std::lock_guard<std::mutex> lock(mutex_);
    TCHAR szHallSvrIP[MAX_SERVERIP_LEN] = {};
    GetPrivateProfileString(_T("hall_server"), _T("ip"), _T(""), szHallSvrIP, sizeof szHallSvrIP, g_szIniFile);
    const auto nHallSvrPort = GetPrivateProfileInt(_T("hall_server"), _T("port"), 0, g_szIniFile);
    if (connection_) {
        LOG_INFO("token = %d", connection_->GetTokenID());
    }
    LOG_INFO("hall ip [%s]", szHallSvrIP);
    LOG_INFO("hall port [%d]", nHallSvrPort);
    LOG_INFO("hall_notify_thread_ [%d]", notify_thread_.GetThreadID());
    LOG_INFO("hall_heart_timer_thread_ [%d]", timer_thread_.GetThreadID());
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
#endif
    return kCommSucc;
}

int RobotHallManager::BriefInfo() const {
    CHECK_MAIN_OR_LAUNCH_THREAD();
#ifdef _DEBUG
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
#endif
    return kCommSucc;
}