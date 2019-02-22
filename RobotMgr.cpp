#include "stdafx.h"
#include "RobotMgr.h"
#include "Main.h"
#include "RobotReq.h"
#include "common_func.h"
#include "RobotUitls.h"
#include "RobotDef.h"
#include "setting_manager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#define ROBOT_APPLY_DEPOSIT_KEY "zjPUYq9L36oA9zke"

int CRobotMgr::Init() {
    main_timer_thread_.Initial(std::thread([this] {this->ThreadMainProc(); }));

    hall_notify_thread_.Initial(std::thread([this] {this->ThreadHallNotify(); }));

    heart_timer_thread_.Initial(std::thread([this] {this->ThreadSendPluse(); }));

    deposit_timer_thread_.Initial(std::thread([this] {this->ThreadDeposit(); }));

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
    heart_timer_thread_.Release();
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


int CRobotMgr::SendHallRequest(RequestID nReqId, uint32_t& nDataLen, void *pData, RequestID &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho /*= true*/, uint32_t wait_ms /*= REQ_TIMEOUT_INTERVAL*/) {
    {
        std::lock_guard<std::mutex> lock(hall_connection_mutex_);
        if (!hall_connection_) {
            UWL_ERR("SendHallRequest m_CoonHall nil ERR_CONNECT_NOT_EXIST nReqId = %d", nReqId);
            assert(false);
            return kCommFaild;
        }

        if (!hall_connection_->IsConnected()) {
            UWL_ERR("SendHallRequest m_CoonHall not connect ERR_CONNECT_DISABLE nReqId = %d", nReqId);
            assert(false);
            return kCommFaild;
        }
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
    Request.pDataPtr = pData;

    BOOL bTimeOut = FALSE, bResult = TRUE;
    {
        std::lock_guard<std::mutex> lock(hall_connection_mutex_);
        bResult = hall_connection_->SendRequest(&Context, &Request, &Response, bTimeOut, wait_ms);
    }

    if (!bResult)///if timeout or disconnect 
    {
        UWL_ERR("SendHallRequest m_ConnHall->SendRequest fail bTimeOut = %d, nReqId = %d", bTimeOut, nReqId);
        assert(false);
        return kCommFaild;
        //TODO
        //std::make_tuple(false, (bTimeOut ? ERR_SENDREQUEST_TIMEOUT : ERR_OPERATE_FAILED));
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


void CRobotMgr::OnHallNotify(RequestID nReqId, void* pDataPtr, int32_t nSize) {
    switch (nReqId) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
            OnDisconnHall(nReqId, pDataPtr, nSize);
            break;
        default:
            break;
    }
}

void CRobotMgr::OnDisconnHall(RequestID nReqId, void* pDataPtr, int32_t nSize) {
    UWL_ERR(_T("与大厅服务断开连接"));
    {
        std::lock_guard<std::mutex> lock(hall_connection_mutex_);
        hall_connection_->DestroyEx();
    }

    //@zhuhangmin 20190222 FixMe 请注意整个函数过程应该是原子性的
    //大厅登陆线程应注意不影响此原子过程
    {
        std::lock_guard<std::mutex> lock(robot_map_mutex_);
        for (auto&& it = robot_map_.begin(); it != robot_map_.end(); it++) {
            it->second->SetLogon(false);
        }
    }
}

void CRobotMgr::ThreadSendPluse() {
    UWL_INF(_T("Hall KeepAlive thread started. id = %d"), GetCurrentThreadId());
    while (TRUE) {
        DWORD dwRet = WaitForSingleObject(g_hExitServer, PluseInterval);
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }

        if (WAIT_TIMEOUT == dwRet) {

            SendHallPluse();

            SendGamePluse();

        }
    }
    UWL_INF(_T("Hall KeepAlive thread exiting. id = %d"), GetCurrentThreadId());
    return;
}

void CRobotMgr::ThreadMainProc() {
    UwlTrace(_T("timer thread started. id = %d"), GetCurrentThreadId());

    while (TRUE) {
        DWORD dwRet = WaitForSingleObject(g_hExitServer, DEF_TIMER_INTERVAL);
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }
        if (WAIT_TIMEOUT == dwRet) { // timeout
            //UWL_DBG(_T("[interval] ---------------------- timer thread triggered. do something. interval = %ld ms."), DEF_TIMER_INTERVAL);
            //UWL_DBG("[interval] TimerThreadProc = %I32u", time(nullptr));

            LogonHall();

            //enter game

        }
    }

    UwlLogFile(_T("timer thread exiting. id = %d"), GetCurrentThreadId());
    return;
}

void	CRobotMgr::ThreadDeposit() {
    UWL_INF(_T("Hall Deposit thread started. id = %d"), GetCurrentThreadId());

    while (TRUE) {
        DWORD dwRet = WaitForSingleObject(g_hExitServer, DepositInterval);
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }

        if (WAIT_TIMEOUT == dwRet) {
            DepositMap deposit_map_bak;
            {
                std::lock_guard<std::mutex> lock(deposit_map_mutex_);
                deposit_map_bak = std::move(deposit_map_);
            }

            for (auto& kv : deposit_map_bak) {
                auto userid = kv.first;
                auto deposit_type = kv.second;

                if (deposit_type == DepositType::kGain) {
                    RobotGainDeposit(userid, GainAmount);
                }

                if (deposit_type == DepositType::kBack) {
                    RobotBackDeposit(userid, BackAmount);
                }
            }
        }
    }

    UWL_INF(_T("Hall Deposit thread exiting. id = %d"), GetCurrentThreadId());
    return;
}


///////////// "具体业务"

int CRobotMgr::RobotGainDeposit(UserID userid, int32_t amount) {
    static TCHAR szHttpUrl[128];
    static int nResult = GetPrivateProfileString(_T("RobotReward"), _T("HttpUrl_G"), _T("null"), szHttpUrl, sizeof(szHttpUrl), g_szIniFile);
    if (0 == strcmp(szHttpUrl, "null"))
        return kCommFaild;

    TCHAR szField[128] = {};
    sprintf_s(szField, "ActiveID_Game%d", g_gameID);
    TCHAR szValue[128] = {};
    GetPrivateProfileString(_T("RobotReward"), szField, _T("null"), szValue, sizeof(szValue), g_szIniFile);
    if (0 == strcmp(szValue, "null"))
        return kCommFaild;

    Json::Value root;
    Json::FastWriter fast_writer;
    root["GameId"] = Json::Value(g_gameID);
    root["ActId"] = Json::Value(szValue);
    root["UserId"] = Json::Value(userid);
    root["UserNickName"] = Json::Value("Robot");
    root["Num"] = Json::Value(amount);
    root["Operation"] = Json::Value(1);
    root["Device"] = Json::Value(xyConvertIntToStr(g_gameID) + "+" + xyConvertIntToStr(userid));
    root["Time"] = Json::Value((LONGLONG) time(NULL) * 1000);
    root["Sign"] = Json::Value(MD5String((root["ActId"].asString() + "|" + root["UserId"].asString() + "|" + root["Time"].asString() + "|" + ROBOT_APPLY_DEPOSIT_KEY).c_str()));

    CString strParam = fast_writer.write(root).c_str();
    CString strResult = RobotUitls::ExecHttpRequestPost(szHttpUrl, strParam);
    Json::Reader reader;
    Json::Value _root;
    if (!reader.parse((LPCTSTR) strResult, _root))
        return kCommFaild;
    if (_root["Code"].asInt() != 0) {
        UWL_ERR("m_Account = %d gain deposit fail, code = %d, strResult = %s", userid, _root["Code"].asInt(), strResult);
        //assert(false);
        return kCommFaild;
    }

    return kCommSucc;
}

int CRobotMgr::RobotBackDeposit(UserID userid, int32_t amount) {
    static TCHAR szHttpUrl[128];
    static int nResult = GetPrivateProfileString(_T("RobotReward"), _T("HttpUrl_C"), _T("null"), szHttpUrl, sizeof(szHttpUrl), g_szIniFile);
    if (0 == strcmp(szHttpUrl, "null"))
        return kCommFaild;

    TCHAR szField[128] = {};
    sprintf_s(szField, "ActiveID_Game%d", g_gameID);
    TCHAR szValue[128] = {};
    GetPrivateProfileString(_T("RobotReward"), szField, _T("null"), szValue, sizeof(szValue), g_szIniFile);
    if (0 == strcmp(szValue, "null"))
        return kCommFaild;

    Json::Value root;
    Json::FastWriter fast_writer;
    root["GameId"] = Json::Value(g_gameID);
    root["ActId"] = Json::Value(szValue);
    root["UserId"] = Json::Value(userid);
    root["UserNickName"] = Json::Value("Robot");
    root["Num"] = Json::Value(amount);
    root["Operation"] = Json::Value(2);
    root["Device"] = Json::Value(xyConvertIntToStr(g_gameID) + "+" + xyConvertIntToStr(userid));
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

int CRobotMgr::LogonHall() {
    //pick up a random robot who is not logon hall
    RobotSetting robot_setting_;

    RobotPtr random_robot;
    for (int i = 0; i < MaxRandomTry; i++) {
        if (kCommFaild == SettingManager::Instance().GetRandomRobotSetting(robot_setting_)) {
            return kCommFaild;
        }
        {
            std::lock_guard<std::mutex> lock(robot_map_mutex_);
            auto random_userid = robot_setting_.userid;
            auto robot = GetRobotWithLock(random_userid);
            if (!robot) {
                robot = std::make_shared<Robot>(random_userid);
                SetRobotWithLock(robot);
            }

            if (robot->IsLogon()) continue;
            random_robot = robot; break;
        }
    }

    if (!random_robot) {
        UWL_WRN("not find robot who can logon");
        return kCommFaild;
    }

    auto userid = random_robot->GetUserID();

    RobotSetting setting;
    if (kCommFaild == SettingManager::Instance().GetRobotSetting(userid, setting)) {
        return kCommFaild;
    }
    std::string password = setting.password;

    //账号对应client是否已经生成, 是否已经登入大厅
    LOGON_USER_V2  logonUser = {};
    logonUser.nUserID = userid;
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
    strcpy_s(logonUser.szPassword, password.c_str());

    RequestID nResponse;
    std::shared_ptr<void> pRetData;
    uint32_t nDataLen = sizeof(logonUser);
    if (kCommFaild == SendHallRequest(GR_LOGON_USER_V2, nDataLen, &logonUser, nResponse, pRetData))
        return kCommFaild;

    if (!(nResponse == GR_LOGON_SUCCEEDED || nResponse == GR_LOGON_SUCCEEDED_V2)) {
        UWL_ERR("ACCOUNT = %d GR_LOGON_USER_V2 FAIL", userid);
        return kCommFaild;
    }

    random_robot->SetLogonData((LPLOGON_SUCCEED_V2) pRetData.get());

    UWL_INF("userid:%d logon hall ok.", userid);
    return kCommSucc;
}

void CRobotMgr::SendHallPluse() {
    HALLUSER_PULSE hp = {};
    hp.nUserID = 0;
    hp.nAgentGroupID = ROBOT_AGENT_GROUP_ID;

    RequestID nRespID = 0;
    std::shared_ptr<void> pRetData;
    uint32_t nDataLen = sizeof(HALLUSER_PULSE);
    auto result = SendHallRequest(GR_HALLUSER_PULSE, nDataLen, &hp, nRespID, pRetData, false);
    if (result == kCommFaild) {
        UWL_ERR("Send hall pluse failed");
    }

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

void CRobotMgr::SetRobotWithLock(RobotPtr client) {
    robot_map_.insert(std::make_pair(client->GetUserID(), client));
}


RobotPtr CRobotMgr::GetRobotByToken(const EConnType& type, const TokenID& id) {
    std::lock_guard<std::mutex> lock(robot_map_mutex_);
    auto it = std::find_if(robot_map_.begin(), robot_map_.end(), [&] (const std::pair<UserID, RobotPtr>& it) {
        return it.second->GetTokenID() == id;
    });

    return it != robot_map_.end() ? it->second : nullptr;
}
