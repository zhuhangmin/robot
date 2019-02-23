#include "stdafx.h"
#include "RobotMgr.h"
#include "Main.h"
#include "common_func.h"
#include "RobotUitls.h"
#include "RobotDef.h"
#include "setting_manager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#define ROBOT_APPLY_DEPOSIT_KEY "zjPUYq9L36oA9zke"

int CRobotMgr::Init() {

    auto setting = SettingManager::Instance().GetRobotSettingMap();
    for (auto& kv : setting) {
        auto userid = kv.first;
        hall_logon_status_map_[userid] = HallLogonStatusType::kNotLogon;
    }

    hall_notify_thread_.Initial(std::thread([this] {this->ThreadHallNotify(); }));

    hall_heart_timer_thread_.Initial(std::thread([this] {this->ThreadHallPluse(); }));

    robot_heart_timer_thread_.Initial(std::thread([this] {this->ThreadRobotPluse(); }));

    robot_notify_thread_.Initial(std::thread([this] {this->ThreadRobotNotify(); }));

    deposit_timer_thread_.Initial(std::thread([this] {this->ThreadDeposit(); }));

    main_timer_thread_.Initial(std::thread([this] {this->ThreadMainProc(); }));

    if (kCommFaild == ConnectHall()) {
        UWL_ERR("ConnectHall() return failed");
        assert(false);
        return kCommFaild;
    }



    UWL_INF("CRobotMgr::Init Sucessed.");
    return kCommSucc;
}
void	CRobotMgr::Term() {
    main_timer_thread_.Release();
    hall_notify_thread_.Release();
    hall_heart_timer_thread_.Release();
    deposit_timer_thread_.Release();
}

int CRobotMgr::ConnectHall(bool bReconn /*= false*/) {
    TCHAR szHallSvrIP[MAX_SERVERIP_LEN] = {};
    GetPrivateProfileString(_T("HallServer"), _T("IP"), _T(""), szHallSvrIP, sizeof(szHallSvrIP), g_szIniFile);
    auto nHallSvrPort = GetPrivateProfileInt(_T("HallServer"), _T("Port"), 0, g_szIniFile);

    {
        std::lock_guard<std::mutex> lock(hall_connection_mutex_);
        hall_connection_->InitKey(KEY_HALL, ENCRYPT_AES, 0);

        if (!hall_connection_->Create(szHallSvrIP, nHallSvrPort, 5, 0, hall_notify_thread_.ThreadId(), 0, GetHelloData(), GetHelloLength())) {
            UWL_ERR("[ROUTE] ConnectHall Faild! IP:%s Port:%d", szHallSvrIP, nHallSvrPort);
            assert(false);
            return kCommFaild;
        }
    }
    UWL_INF("ConnectHall[ReConn:%d] OK! IP:%s Port:%d", (int) bReconn, szHallSvrIP, nHallSvrPort);
    return kCommSucc;
}


int CRobotMgr::SendHallRequestWithLock(RequestID requestid, int& data_size, void *req_data_ptr, RequestID &response_id, std::shared_ptr<void> &resp_data_ptr, bool need_echo /*= true*/) {
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

    BOOL bTimeOut = FALSE, bResult = TRUE;
    bResult = hall_connection_->SendRequest(&Context, &Request, &Response, bTimeOut, RequestTimeOut);

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
        CHAR info[512] = {};
        sprintf_s(info, "%s", resp_data_ptr);
        data_size = 0;
        UWL_ERR("SendHallRequest m_ConnHall->SendRequest fail responseid GR_ERROR_INFOMATION nReqId = %d", requestid);
        assert(false);
        return kCommFaild;
    }
    return kCommSucc;
}


void	CRobotMgr::ThreadHallNotify() {
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


void CRobotMgr::OnHallNotify(RequestID requestid, void* ntf_data_ptr, int data_size) {
    switch (requestid) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
            OnDisconnHall(requestid, ntf_data_ptr, data_size);
            break;
        default:
            break;
    }
}

void CRobotMgr::OnDisconnHall(RequestID requestid, void* data_ptr, int data_size) {
    UWL_ERR(_T("与大厅服务断开连接"));
    std::lock_guard<std::mutex> lock(hall_connection_mutex_);
    hall_connection_->DestroyEx();
    for (auto& kv : hall_logon_status_map_) {
        auto userid = kv.first;
        SetLogonStatusWithLock(userid, HallLogonStatusType::kNotLogon);
    }
}

void CRobotMgr::ThreadHallPluse() {
    UWL_INF(_T("Hall KeepAlive thread started. id = %d"), GetCurrentThreadId());
    while (true) {
        DWORD dwRet = WaitForSingleObject(g_hExitServer, PluseInterval);
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }

        if (WAIT_TIMEOUT == dwRet) {

            SendHallPluse();


            auto setting = SettingManager::Instance().GetRoomSettingMap();
            for (auto& kv : setting) {
                auto roomid = kv.first;
                if (kCommFaild == SendGetRoomData(roomid)) {
                    assert(false);
                    UWL_ERR("cannnot get hall room data = %d", roomid);
                }
            }


        }
    }
    UWL_INF(_T("Hall KeepAlive thread exiting. id = %d"), GetCurrentThreadId());
    return;
}

void CRobotMgr::ThreadMainProc() {
    UwlTrace(_T("timer thread started. id = %d"), GetCurrentThreadId());
    while (true) {
        DWORD dwRet = WaitForSingleObject(g_hExitServer, SettingManager::Instance().GetMainsInterval());
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }
        if (WAIT_TIMEOUT == dwRet) { // timeout
            //UWL_DBG(_T("[interval] ---------------------- timer thread triggered. do something. interval = %ld ms."), DEF_TIMER_INTERVAL);
            //UWL_DBG("[interval] TimerThreadProc = %I32u", time(nullptr));

            //TODO 随机选一个没有进入游戏的robot
            //FixMe：
            UserID random_userid = InvalidUserID;
            if (kCommFaild == GetRandomNotLogonUserID(random_userid))
                continue;

            if (InvalidUserID == random_userid)
                continue;

            if (kCommFaild == LogonHall(random_userid))
                continue;


            RobotPtr robot;
            if (robot_map_.find(random_userid) == robot_map_.end()) {
                robot_map_[random_userid] = std::make_shared<Robot>(random_userid);
            }
            robot = robot_map_[random_userid];

            //TODO designed roomid
            RoomID designed_roomid = 7846; //FixMe: hard code
            HallRoomData hall_room_data;
            if (kCommFaild == GetHallRoomData(designed_roomid, hall_room_data))
                continue;

            //@zhuhangmin 20190223 issue: 网络库不支持域名IPV6解析，使用配置IP
            auto game_ip = SettingManager::Instance().GetGameIP();
            auto game_port = hall_room_data.room.nPort;
            auto game_notify_thread_id = robot_notify_thread_.ThreadId();
            if (kCommFaild == robot->ConnectGame(game_ip, game_port, game_notify_thread_id))
                continue;

            if (kCommFaild == robot->SendEnterGame(designed_roomid))
                continue;

            //TODO 
            // HANDLE EXCEPTION
            // DEPOSIT OVERFLOW UNDERFLOW

        }
    }

    UwlLogFile(_T("timer thread exiting. id = %d"), GetCurrentThreadId());
    return;
}

void	CRobotMgr::ThreadDeposit() {
    UWL_INF(_T("Hall Deposit thread started. id = %d"), SettingManager::Instance().GetDepositInterval());

    while (true) {
        DWORD dwRet = WaitForSingleObject(g_hExitServer, DepositInterval);
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }

        if (WAIT_TIMEOUT == dwRet) {
            DepositMap deposit_map_temp;
            {
                std::lock_guard<std::mutex> lock(deposit_map_mutex_);
                deposit_map_temp = std::move(deposit_map_);
            }

            for (auto& kv : deposit_map_temp) {
                auto userid = kv.first;
                auto deposit_type = kv.second;

                if (deposit_type == DepositType::kGain) {
                    RobotGainDeposit(userid, SettingManager::Instance().GetGainAmount());
                }

                if (deposit_type == DepositType::kBack) {
                    RobotBackDeposit(userid, SettingManager::Instance().GetBackAmount());
                }
            }
        }
    }

    UWL_INF(_T("Hall Deposit thread exiting. id = %d"), GetCurrentThreadId());
    return;
}


///////////// "具体业务"

int CRobotMgr::RobotGainDeposit(UserID userid, int amount) {
    static TCHAR szHttpUrl[128];
    static int nResult = GetPrivateProfileString(_T("RobotReward"), _T("HttpUrl_G"), _T("null"), szHttpUrl, sizeof(szHttpUrl), g_szIniFile);
    if (0 == strcmp(szHttpUrl, "null"))
        return kCommFaild;

    TCHAR szField[128] = {};
    GameID gameid = SettingManager::Instance().GetGameID();
    sprintf_s(szField, "ActiveID_Game%d", gameid);
    TCHAR szValue[128] = {};
    GetPrivateProfileString(_T("RobotReward"), szField, _T("null"), szValue, sizeof(szValue), g_szIniFile);
    if (0 == strcmp(szValue, "null"))
        return kCommFaild;

    Json::Value root;
    Json::FastWriter fast_writer;
    root["GameId"] = Json::Value(gameid);
    root["ActId"] = Json::Value(szValue);
    root["UserId"] = Json::Value(userid);
    root["UserNickName"] = Json::Value("Robot");
    root["Num"] = Json::Value(amount);
    root["Operation"] = Json::Value(1);
    root["Device"] = Json::Value(xyConvertIntToStr(gameid) + "+" + xyConvertIntToStr(userid));
    root["Time"] = Json::Value((LONGLONG) time(NULL) * 1000);
    root["Sign"] = Json::Value(MD5String((root["ActId"].asString() + "|" + root["UserId"].asString() + "|" + root["Time"].asString() + "|" + ROBOT_APPLY_DEPOSIT_KEY).c_str()));

    CString strParam = fast_writer.write(root).c_str();
    CString strResult = RobotUitls::ExecHttpRequestPost(szHttpUrl, strParam);
    Json::Reader reader;
    Json::Value _root;
    if (!reader.parse((LPCTSTR) strResult, _root))
        return kCommFaild;
    if (_root["Code"].asInt() != 0) {
        UWL_ERR("userid = %d gain deposit fail, code = %d, strResult = %s", userid, _root["Code"].asInt(), strResult);
        //assert(false);
        return kCommFaild;
    }

    return kCommSucc;
}

int CRobotMgr::RobotBackDeposit(UserID userid, int amount) {
    static TCHAR szHttpUrl[128];
    static int nResult = GetPrivateProfileString(_T("RobotReward"), _T("HttpUrl_C"), _T("null"), szHttpUrl, sizeof(szHttpUrl), g_szIniFile);
    if (0 == strcmp(szHttpUrl, "null"))
        return kCommFaild;

    TCHAR szField[128] = {};
    GameID gameid = SettingManager::Instance().GetGameID();
    sprintf_s(szField, "ActiveID_Game%d", gameid);
    TCHAR szValue[128] = {};
    GetPrivateProfileString(_T("RobotReward"), szField, _T("null"), szValue, sizeof(szValue), g_szIniFile);
    if (0 == strcmp(szValue, "null"))
        return kCommFaild;

    Json::Value root;
    Json::FastWriter fast_writer;
    root["GameId"] = Json::Value(gameid);
    root["ActId"] = Json::Value(szValue);
    root["UserId"] = Json::Value(userid);
    root["UserNickName"] = Json::Value("Robot");
    root["Num"] = Json::Value(amount);
    root["Operation"] = Json::Value(2);
    root["Device"] = Json::Value(xyConvertIntToStr(gameid) + "+" + xyConvertIntToStr(userid));
    root["Time"] = Json::Value((LONGLONG) time(NULL) * 1000);
    root["Sign"] = Json::Value(MD5String((root["ActId"].asString() + "|" + root["UserId"].asString() + "|" + root["Time"].asString() + "|" + ROBOT_APPLY_DEPOSIT_KEY).c_str()));

    CString strParam = fast_writer.write(root).c_str();
    CString strResult = RobotUitls::ExecHttpRequestPost(szHttpUrl, strParam);
    Json::Reader reader;
    Json::Value _root;
    if (!reader.parse((LPCTSTR) strResult, _root))
        return kCommFaild;
    if (!_root["Status"].asBool())
        return kCommFaild;
    return kCommSucc;
}

int CRobotMgr::LogonHall(UserID userid) {
    std::lock_guard<std::mutex> lock(hall_connection_mutex_);

    RobotSetting setting;
    if (kCommFaild == SettingManager::Instance().GetRobotSetting(userid, setting)) {
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

void CRobotMgr::SendHallPluse() {
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
    }

}

int CRobotMgr::SendGetRoomData(RoomID roomid) {
    std::lock_guard<std::mutex> lock(hall_connection_mutex_);
    GameID gameid = SettingManager::Instance().GetGameID();
    GET_ROOM  gr = {};
    gr.nGameID = gameid;
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

void CRobotMgr::SendGamePluse() {
    std::lock_guard<std::mutex> lock(robot_map_mutex_);
    for (auto& kv : robot_map_) {
        auto robot = kv.second;
        robot->SendGamePulse();
    }
}

RobotPtr CRobotMgr::GetRobotWithLock(UserID userid) {
    if (robot_map_.find(userid) != robot_map_.end()) {
        return robot_map_[userid];
    }
    return nullptr;
}

void CRobotMgr::SetRobotWithLock(RobotPtr robot) {
    robot_map_.insert(std::make_pair(robot->GetUserID(), robot));
}

RobotPtr CRobotMgr::GetRobotByTokenWithLock(const TokenID& id) {
    auto it = std::find_if(robot_map_.begin(), robot_map_.end(), [&] (const std::pair<UserID, RobotPtr>& it) {
        return it.second->GetTokenID() == id;
    });

    return it != robot_map_.end() ? it->second : nullptr;
}

int CRobotMgr::GetLogonStatusWithLock(const UserID& userid, HallLogonStatusType& status) {
    if (hall_logon_status_map_.find(userid) == hall_logon_status_map_.end()) {
        hall_logon_status_map_.insert(std::make_pair(userid, HallLogonStatusType::kNotLogon));
    }

    status = hall_logon_status_map_[userid];
    return kCommSucc;
}

void CRobotMgr::SetLogonStatusWithLock(const UserID userid, HallLogonStatusType status) {
    hall_logon_status_map_[userid] = status;
}

int CRobotMgr::GetDepositTypeWithLock(const UserID& userid, DepositType& type) {
    if (deposit_map_.find(userid) == deposit_map_.end()) {
        return kCommFaild;
    }

    type = deposit_map_[userid];
    return kCommSucc;
}

void CRobotMgr::SetDepositTypesWithLock(const UserID userid, DepositType type) {
    deposit_map_[userid] = type;
}

int CRobotMgr::GetHallRoomData(const RoomID& roomid, HallRoomData& hall_room_data) {
    std::lock_guard<std::mutex> lock(hall_connection_mutex_);
    return GetHallRoomDataWithLock(roomid, hall_room_data);
}

int CRobotMgr::GetHallRoomDataWithLock(const RoomID& roomid, HallRoomData& hall_room_data) {
    if (hall_room_data_map_.find(roomid) == hall_room_data_map_.end()) {
        return kCommFaild;
    }

    hall_room_data = hall_room_data_map_[roomid];
    return kCommSucc;
}

void CRobotMgr::SetHallRoomDataWithLock(const RoomID roomid, HallRoomData* hall_room_data) {
    hall_room_data_map_[roomid] = *hall_room_data;
}

int CRobotMgr::GetRandomNotLogonUserID(UserID& random_userid) {
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
    if (kCommFaild == RobotUitls::GenRandInRange(0, temp_map.size() - 1, random_pos)) {
        return kCommFaild;
    }

    auto random_it = std::next(std::begin(temp_map), random_pos);
    random_userid = random_it->first;
    return kCommSucc;
}

void CRobotMgr::ThreadRobotPluse() {
    UWL_INF(_T("Robot KeepAlive thread started. id = %d"), GetCurrentThreadId());
    while (true) {
        DWORD dwRet = WaitForSingleObject(g_hExitServer, PluseInterval);
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }

        if (WAIT_TIMEOUT == dwRet) {

            SendGamePluse();

        }
    }
    UWL_INF(_T("Robot KeepAlive thread exiting. id = %d"), GetCurrentThreadId());
    return;
}

void CRobotMgr::ThreadRobotNotify() {
    UWL_INF(_T("Robot Notify thread started. id = %d"), GetCurrentThreadId());

    MSG msg = {};
    while (GetMessage(&msg, 0, 0, 0)) {
        if (UM_DATA_RECEIVED == msg.message) {

            LPCONTEXT_HEAD	pContext = (LPCONTEXT_HEAD) (msg.wParam);
            LPREQUEST		pRequest = (LPREQUEST) (msg.lParam);

            TokenID		nTokenID = pContext->lTokenID;
            RequestID		nReqstID = pRequest->head.nRequest;

            OnRobotNotify(nReqstID, pRequest->pDataPtr, pRequest->nDataLen, nTokenID);

            SAFE_DELETE(pContext);
            UwlClearRequest(pRequest);
            SAFE_DELETE(pRequest);
        } else {
            DispatchMessage(&msg);
        }
    }
    UWL_INF(_T("Robot Notify thread exiting. id = %d"), GetCurrentThreadId());
    return;
}

void CRobotMgr::OnRobotNotify(RequestID requestid, void* ntf_data_ptr, int data_size, TokenID token_id) {
    RobotPtr robot;
    {
        std::lock_guard<std::mutex> lock(robot_map_mutex_);
        robot = GetRobotByTokenWithLock(token_id);
    }

    if (!robot) {
        assert(false);
        UWL_WRN(_T("GameNotify robot not found. nTokenID = %d reqId:%d"), token_id, requestid);
        return;
    }

    switch (requestid) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
            robot->DisconnectGame();
            break;
        default:
            break;
    }
}
