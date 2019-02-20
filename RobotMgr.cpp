#include "stdafx.h"
#include "RobotMgr.h"
#include "Main.h"
#include "RobotReq.h"
#include "common_func.h"
#include "RobotUitls.h"
#include "RobotDef.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#define ROBOT_APPLY_DEPOSIT_KEY "zjPUYq9L36oA9zke"

bool	CRobotMgr::Init() {
    main_timer_thread_.Initial(std::thread([this] {this->ThreadMainProc(); }));

    hall_notify_thread_.Initial(std::thread([this] {this->ThreadHallNotify(); }));

    heart_timer_thread_.Initial(std::thread([this] {this->ThreadSendHallPluse(); }));

    deposit_timer_thread_.Initial(std::thread([this] {this->ThreadDeposit(); }));

    if (!ConnectHall()) {
        UWL_ERR("ConnectHall() return false");
        assert(false);
        return false;
    }

    UWL_INF("CRobotMgr::Init Sucessed.");
    return true;
}
void	CRobotMgr::Term() {
    main_timer_thread_.Release();
    hall_notify_thread_.Release();
    heart_timer_thread_.Release();
    deposit_timer_thread_.Release();
}

bool	CRobotMgr::ConnectHall(bool bReconn) {
    TCHAR szHallSvrIP[MAX_SERVERIP_LEN] = {};
    GetPrivateProfileString(_T("HallServer"), _T("IP"), _T(""), szHallSvrIP, sizeof(szHallSvrIP), g_szIniFile);
    auto nHallSvrPort = GetPrivateProfileInt(_T("HallServer"), _T("Port"), 0, g_szIniFile);

    {
        std::lock_guard<std::mutex> lock(hall_connection_mutex_);
        hall_connection_->InitKey(KEY_HALL, ENCRYPT_AES, 0);

        if (!hall_connection_->Create(szHallSvrIP, nHallSvrPort, 5, 0, hall_notify_thread_.ThreadId(), 0, GetHelloData(), GetHelloLength())) {
            UWL_ERR("[ROUTE] ConnectHall Faild! IP:%s Port:%d", szHallSvrIP, nHallSvrPort);
            assert(false);
            return false;
        }
    }
    UWL_INF("ConnectHall[ReConn:%d] OK! IP:%s Port:%d", (int) bReconn, szHallSvrIP, nHallSvrPort);
    return true;
}


int CRobotMgr::SendHallRequest(RequestID nReqId, uint32_t& nDataLen, void *pData, RequestID &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho /*= true*/, uint32_t wait_ms /*= REQ_TIMEOUT_INTERVAL*/) {
    std::lock_guard<std::mutex> lock(hall_connection_mutex_);
    if (!hall_connection_) {
        UWL_ERR("SendHallRequest m_CoonHall nil ERR_CONNECT_NOT_EXIST nReqId = %d", nReqId);
        assert(false);
        return false;
    }

    if (!hall_connection_->IsConnected()) {
        UWL_ERR("SendHallRequest m_CoonHall not connect ERR_CONNECT_DISABLE nReqId = %d", nReqId);
        assert(false);
        return false;
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
        return false;
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
        return false;
    }
    return true;
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
    std::lock_guard<std::mutex> lock(hall_connection_mutex_);
    switch (nReqId) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
            OnDisconnHallWithLock(nReqId, pDataPtr, nSize);
            break;
        default:
            break;
    }
}

void CRobotMgr::OnDisconnHallWithLock(RequestID nReqId, void* pDataPtr, int32_t nSize) {
    UWL_ERR(_T("与大厅服务断开连接"));
    hall_connection_->DestroyEx();
    {
        for (auto&& it = robot_map_.begin(); it != robot_map_.end(); it++) {
            it->second->SetLogon(false);
        }
    }
}

void CRobotMgr::ThreadSendHallPluse() {
    UWL_INF(_T("Hall KeepAlive thread started. id = %d"), GetCurrentThreadId());

    auto now_time = time(nullptr);
    static time_t	last_pluse_time = now_time;
    if (now_time - last_pluse_time >= PluseInterval)
        last_pluse_time = now_time;
    else
        return;

    HALLUSER_PULSE hp = {};
    hp.nUserID = 0;
    hp.nAgentGroupID = ROBOT_AGENT_GROUP_ID;

    RequestID nRespID = 0;
    std::shared_ptr<void> pRetData;
    uint32_t nDataLen = sizeof(HALLUSER_PULSE);
    SendHallRequest(GR_HALLUSER_PULSE, nDataLen, &hp, nRespID, pRetData, false);

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
            //logon 

            //enter game
            //OnTimerLogonHall(nCurrTime);
            /*   OnTimerSendHallPluse(nCurrTime);
            OnTimerSendGamePluse(nCurrTime);*/
            //OnTimerUpdateDeposit(nCurrTime);
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
            {
                std::lock_guard<std::mutex> lock(hall_connection_mutex_);
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
            }

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
        return false;

    TCHAR szField[128] = {};
    sprintf_s(szField, "ActiveID_Game%d", g_gameID);
    TCHAR szValue[128] = {};
    GetPrivateProfileString(_T("RobotReward"), szField, _T("null"), szValue, sizeof(szValue), g_szIniFile);
    if (0 == strcmp(szValue, "null"))
        return false;

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
        return false;
    if (_root["Code"].asInt() != 0) {
        UWL_ERR("m_Account = %d gain deposit fail, code = %d, strResult = %s", userid, _root["Code"].asInt(), strResult);
        //assert(false);
        return false;
    }

    return true;
}

int CRobotMgr::RobotBackDeposit(UserID userid, int32_t amount) {
    static TCHAR szHttpUrl[128];
    static int nResult = GetPrivateProfileString(_T("RobotReward"), _T("HttpUrl_C"), _T("null"), szHttpUrl, sizeof(szHttpUrl), g_szIniFile);
    if (0 == strcmp(szHttpUrl, "null"))
        return false;

    TCHAR szField[128] = {};
    sprintf_s(szField, "ActiveID_Game%d", g_gameID);
    TCHAR szValue[128] = {};
    GetPrivateProfileString(_T("RobotReward"), szField, _T("null"), szValue, sizeof(szValue), g_szIniFile);
    if (0 == strcmp(szValue, "null"))
        return false;

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
        return false;
    if (!_root["Status"].asBool())
        return false;
    return true;
}


int CRobotMgr::RobotLogonHall(const int32_t& account /*= 0*/) {

    //AccountSettingMap::iterator itRobot = account_setting_map_.end();

    ////配置账号是否存在
    //// 指定账号 否则随机
    //if (account != 0) {
    //    auto&& it = account_setting_map_.find(account); // robot setting
    //    if (it == account_setting_map_.end())
    //        return std::make_tuple(false, "使用无效的机器人账号");

    //    if (IsLogon(account))
    //        return std::make_tuple(true, "不可重复登录");

    //    itRobot = it;
    //} else {
    //    //random
    //    for (int i = 0; i < 10; i++) {
    //        auto it = account_setting_map_.begin();
    //        auto randnum = rand();
    //        auto mapSize = account_setting_map_.size();
    //        auto num = randnum % mapSize;
    //        std::advance(it, num);
    //        if (IsLogon(it->first)) continue;
    //        itRobot = it; break;
    //    }

    //}

    //if (itRobot == account_setting_map_.end()) {

    //    if (account == 0) {
    //        //UWL_DBG("没有剩余机器人账号数据可用于登陆大厅");
    //    } else {
    //        UWL_ERR("没有找到机器人账号数据 account = %d", account);
    //        assert(false);
    //    }
    //    return std::make_tuple(true, "没有找到机器人账号数据可用于登陆大厅");
    //}

    ////账号对应client是否已经生成, 是否已经登入大厅
    //auto client = GetRobotClient(itRobot->first);
    //if (!client)
    //    client = std::make_shared<Robot>(itRobot->second);

    //if (client->IsLogon())
    //    return std::make_tuple(false, "已经登录");

    //LOGON_USER_V2  logonUser = {};
    //logonUser.nUserID = client->GetUserID();
    //xyGetHardID(logonUser.szHardID);  // 硬件ID
    //xyGetVolumeID(logonUser.szVolumeID);
    //xyGetMachineID(logonUser.szMachineID);
    //UwlGetSystemVersion(logonUser.dwSysVer);
    //logonUser.nAgentGroupID = ROBOT_AGENT_GROUP_ID; // 使用888作为组号
    //logonUser.dwLogonFlags |= (FLAG_LOGON_INTER | FLAG_LOGON_ROBOT_TOOL);
    //logonUser.nLogonSvrID = 0;
    //logonUser.nHallBuildNO = 20160414;
    //logonUser.nHallNetDelay = 1;
    //logonUser.nHallRunCount = 1;
    //strcpy_s(logonUser.szPassword, client->Password().c_str());

    //RequestID nResponse;
    ////LPVOID	 pRetData = NULL;
    //std::shared_ptr<void> pRetData;
    //uint32_t nDataLen = sizeof(logonUser);
    //auto it = SendHallRequest(GR_LOGON_USER_V2, nDataLen, &logonUser, nResponse, pRetData);
    //if (!TUPLE_ELE(it, 0)) {
    //    UWL_ERR("ACCOUNT = %d GR_LOGON_USER_V2 FAIL", client->GetUserID());
    //    return it;
    //}

    //if (!(nResponse == GR_LOGON_SUCCEEDED || nResponse == GR_LOGON_SUCCEEDED_V2)) {
    //    UWL_ERR("ACCOUNT = %d GR_LOGON_USER_V2 FAIL", client->GetUserID());
    //    return std::make_tuple(false, std::to_string(nResponse));
    //}

    //client->SetLogon(true);
    //client->SetLogonData((LPLOGON_SUCCEED_V2) pRetData.get());
    //SetRobotClient(client);

    //UWL_INF("account:%d userid:%d logon hall ok.", client->GetUserID(), client->GetUserID());
    //return std::make_tuple(true, ERR_OPERATE_SUCESS);
    return true;
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


void CRobotMgr::SetRoomID(UserID userid, RoomID roomid) {
    /*if (robot_map_.find(userid) != robot_map_.end()) {
    robot_map_[userid]->SetRoomID(roomid);
    }*/
}

int CRobotMgr::GetRoomCurrentRobotSize(RoomID roomid) {
    return 0;
}


bool	CRobotMgr::OnTimerLogonHall(time_t now_time) {
    //    if (!hall_connection_) {
    //        UWL_ERR("SendHallRequest OnTimerReconnectHall nil ERR_CONNECT_NOT_EXIST");
    //        assert(false);
    //        return false;
    //    }
    //
    //
    //    if (hall_connection_->IsConnected()) return true;
    //
    //    //@zhuhangmin 10s重连代码，200+机器人 100s 重连 2000多次, 回导致短时间大厅登陆压力过大
    //#define MAIN_RECONNECT_HALL_GAP_TIME (60) // 60s
    //    static time_t	sLastReconnectHallGapTime = nCurrTime;
    //    if (nCurrTime - sLastReconnectHallGapTime >= MAIN_RECONNECT_HALL_GAP_TIME)
    //        sLastReconnectHallGapTime = nCurrTime;
    //    else return false;
    //
    //    if (!InitConnectHall()) {
    //        UWL_ERR("InitConnectHall");
    //        assert(false);
    //        return false;
    //    }
    //
    //    int32_t nCount = 0;
    //    std::vector<int32_t> vecAccounts;
    //    {
    //        for (auto&& it = account_setting_map_.begin(); it != account_setting_map_.end(); it++) {
    //            if (!IsLogon(it->first)) continue;
    //
    //            if (++nCount >= 5)		 break;
    //
    //            vecAccounts.push_back(it->second.account);
    //        }
    //    }
    //    for (auto&& it : vecAccounts) {
    //        //UWL_INF("OnTimerReconnectHall CALL RobotLogonHall");
    //        auto&& tRet = RobotLogonHall(it);
    //        if (!TUPLE_ELE(tRet, 0)) {
    //            assert(false);
    //            UWL_WRN("OnTimerReconnectHall Account:%d logon Err:%s", it, TUPLE_ELE_C(tRet, 1));
    //        }
    //    }
    return true;
}

void    CRobotMgr::OnTimerUpdateDeposit(time_t now_time) {
    //    if (!hall_connection_) {
    //        UWL_ERR("SendHallRequest OnTimerCtrlRoomActiv nil ERR_CONNECT_NOT_EXIST nReqId");
    //        assert(false);
    //        return;
    //    }
    //
    //
    //    if (!hall_connection_->IsConnected()) {
    //        UWL_ERR("SendHallRequest OnTimerUpdateDeposit not connect ERR_CONNECT_DISABLE");
    //        assert(false);
    //        return;
    //    }
    //
    //#define UPDATE_CLIENTNUM_PRE_ONCE	(15)
    //#define MAIN_ROOM_UPDATE_DEPOSIT_GAP_TIME (15) // 15s
    //    static time_t	sLastRoomUpdateDepositGapTime = 0;
    //    if (nCurrTime - sLastRoomUpdateDepositGapTime >= MAIN_ROOM_UPDATE_DEPOSIT_GAP_TIME)
    //        sLastRoomUpdateDepositGapTime = nCurrTime;
    //    else return;
    //
    //    int nCount = 0;
    //    {
    //        std::lock_guard<std::mutex> lock(robot_map_mutex_);
    //        for (auto&& it = robot_map_.begin(); it != robot_map_.end(); it++) {
    //            if (!it->second->IsLogon())	continue;
    //
    //            //if (it->second->GetRoomID() != 0)	continue;
    //
    //            //if (it->second->IsGaming())		continue;
    //
    //            if (it->second->m_bGainDeposit) {
    //                it->second->m_bGainDeposit = false;
    //                auto&& tRet = RobotGainDeposit(it->second);
    //                if (!TUPLE_ELE(tRet, 0)) {
    //                    UWL_WRN("Account:%d GainDeposit Err:%s", it->second->GetUserID(), TUPLE_ELE_C(tRet, 1));
    //                    continue;
    //                }
    //                nCount++;
    //            }
    //            if (it->second->m_bBackDeposit) {
    //                it->second->m_bBackDeposit = false;
    //                auto&& tRet = RobotBackDeposit(it->second);
    //                if (!TUPLE_ELE(tRet, 0)) {
    //                    UWL_WRN("Account:%d BackDeposit Err:%s", it->second->GetUserID(), TUPLE_ELE_C(tRet, 1));
    //                    continue;
    //                }
    //                nCount++;
    //            }
    //            if (nCount >= UPDATE_CLIENTNUM_PRE_ONCE)
    //                break;
    //        }
    //    }
}
RobotPtr CRobotMgr::GetRobotByToken(const EConnType& type, const TokenID& id) {
    std::lock_guard<std::mutex> lock(robot_map_mutex_);
    auto it = std::find_if(robot_map_.begin(), robot_map_.end(), [&] (const std::pair<UserID, RobotPtr>& it) {
        return it.second->GameToken() == id;
    });

    return it != robot_map_.end() ? it->second : nullptr;
}
