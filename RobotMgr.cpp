#include "stdafx.h"
#include "RobotMgr.h"
#include "Main.h"
#include "RobotReq.h"
#include "common_func.h"
#include "RobotUitls.h"

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
    UWL_INF("InitSetting Account Count = %d", account_setting_map_.size());

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

    if (kCommFaild == InitConnectGame()) {
        UWL_ERR("InitConnectGame failed");
        assert(false);
        return false;
    }

    return true;
}
void	CRobotMgr::Term() {
    for (auto&& it : m_thrdEnterGames)
        it.Release();

    m_thrdHallNotify.Release();
    m_thrdGameNotify.Release();
    m_thrdGameInfoNotify.Release();
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
        room_setting_map_[nRoomId] = RoomSetiing{nRoomId, nCtrlMode, nCtrlValue};
    }

    auto robots = root["robots"];
    size = robots.size();
    for (int i = 0; i < size; ++i) {
        auto robot = robots[i];
        int32_t		nAccount = robot["Account"].asInt();
        std::string sPassword = robot["Password"].asString();
        std::string sNickName = robot["NickName"].asString();
        std::string sPortrait = robot["Portrait"].asString();

        RobotSetting unit;
        unit.account = nAccount;
        unit.password = sPassword;
        unit.nickName = sNickName;
        unit.portraitUrl = sPortrait;
        account_setting_map_[nAccount] = unit;
    }
    return closeRet(true);
}
bool	CRobotMgr::InitNotifyThreads() {
    m_thrdHallNotify.Initial(std::thread([this] {this->ThreadRunHallNotify(); }));
    m_thrdGameNotify.Initial(std::thread([this] {this->ThreadRunGameNotify(); }));
    m_thrdGameInfoNotify.Initial(std::thread([this] {this->ThreadRunGameNotify(); }));
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
        std::lock_guard<std::mutex> lock(hall_connection_mutex_);
        hall_connection_->InitKey(KEY_HALL, ENCRYPT_AES, 0);

        if (!hall_connection_->Create(szHallSvrIP, nHallSvrPort, 5, 0, m_thrdHallNotify.ThreadId(), 0, GetHelloData(), GetHelloLength())) {
            UWL_ERR("[ROUTE] ConnectHall Faild! IP:%s Port:%d", szHallSvrIP, nHallSvrPort);
            assert(false);
            return false;
        }
    }
    UWL_INF("ConnectHall[ReConn:%d] OK! IP:%s Port:%d", (int) bReconn, szHallSvrIP, nHallSvrPort);
    return true;
}

int CRobotMgr::InitConnectGame() {
    TCHAR szGameSvrIP[MAX_SERVERIP_LEN] = {};
    GetPrivateProfileString(_T("GameServer"), _T("IP"), _T(""), szGameSvrIP, sizeof(szGameSvrIP), g_szIniFile);
    auto nGameSvrPort = GetPrivateProfileInt(_T("GameServer"), _T("Port"), 0, g_szIniFile);

    {
        std::lock_guard<std::mutex> lock(game_info_connection_mutex_);
        game_info_connection_->InitKey(KEY_GAMESVR_2_0, ENCRYPT_AES, 0);

        if (!game_info_connection_->Create(szGameSvrIP, nGameSvrPort, 5, 0, m_thrdGameInfoNotify.ThreadId(), 0, GetHelloData(), GetHelloLength())) {
            UWL_ERR("[ROUTE] ConnectGame Faild! IP:%s Port:%d", szGameSvrIP, nGameSvrPort);
            assert(false);
            return kCommFaild;
        }
        UWL_INF("ConnectGame OK! IP:%s Port:%d", szGameSvrIP, nGameSvrPort);

        auto ret = SendValidateReq();

        if (ret == kCommFaild) return ret;

        UWL_INF("SendValidateReq OK!");

        ret = SendGetGameInfo();

        if (ret == kCommFaild) return ret;

        UWL_INF("SendGetGameInfo OK!");

    }


    return kCommSucc;
}


int CRobotMgr::SendValidateReq() {
    std::lock_guard<std::mutex> lock(game_info_connection_mutex_);

    game::base::RobotSvrValidateReq req;
    req.set_client_id(g_nClientID);
    REQUEST response = {};
    auto result = RobotUitls::SendRequest(game_info_connection_, GR_VALID_ROBOTSVR, req, response);

    if (kCommSucc != result) {
        UWL_ERR("SendValidateReq failed");
        return kCommFaild;
    }

    game::base::RobotSvrValidateResp resp;
    int ret = ParseFromRequest(response, resp);
    if (kCommSucc != ret) {
        UWL_ERR("ParseFromRequest faild.");
        return kCommFaild;
    }

    if (kCommSucc != resp.code()) {
        UWL_ERR("SendValidateReq faild. check return[%d]. req = %s", resp.code(), GetStringFromPb(req).c_str());
        return kCommFaild;
    }

    return kCommSucc;
}

// 机器人服务获取游戏内所有玩家信息(room、table、user)
int CRobotMgr::SendGetGameInfo(RoomID roomid /*= 0*/) {
    std::lock_guard<std::mutex> lock(game_info_connection_mutex_);

    game::base::GetGameUsersReq req;
    req.set_clientid(g_nClientID);
    req.set_roomid(roomid);
    REQUEST response = {};
    auto result = RobotUitls::SendRequest(game_info_connection_, GR_GET_GAMEUSERS, req, response);

    if (kCommSucc != result) {
        UWL_ERR("SendGetGameInfo failed");
        return kCommFaild;
    }

    game::base::GetGameUsersResp resp;
    int ret = ParseFromRequest(response, resp);
    if (kCommSucc != ret) {
        UWL_ERR("ParseFromRequest faild.");
        return kCommFaild;
    }

    if (kCommSucc != resp.code()) {
        UWL_ERR("SendGetGameInfo faild. check return[%d]. req = %s", resp.code(), GetStringFromPb(req).c_str());
        return kCommFaild;
    }

    room_map_.clear();
    for (int index = 0; index < resp.rooms_size(); index++) {
        game::base::Room room = resp.rooms(index);
        RoomID roomid = room.room_data().roomid();
        room_map_.insert(std::make_pair(roomid, room));
    }

    user_map_.clear();
    for (int index = 0; index < resp.users_size(); index++) {
        game::base::User user = resp.users(index);
        UserID userid = user.userid();
        user_map_.insert(std::make_pair(userid, user));
    }
    return kCommSucc;
}

TTueRet CRobotMgr::SendHallRequest(RequestID nReqId, uint32_t& nDataLen, void *pData, RequestID &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho /*= true*/, uint32_t wait_ms /*= REQ_TIMEOUT_INTERVAL*/) {
    if (!hall_connection_) {
        UWL_ERR("SendHallRequest m_CoonHall nil ERR_CONNECT_NOT_EXIST nReqId = %d", nReqId);
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_NOT_EXIST);
    }

    if (!hall_connection_->IsConnected()) {
        UWL_ERR("SendHallRequest m_CoonHall not connect ERR_CONNECT_DISABLE nReqId = %d", nReqId);
        assert(false);
        return std::make_tuple(false, ERR_CONNECT_DISABLE);
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
    Request.pDataPtr = pData;//////////////

    BOOL bTimeOut = FALSE, bResult = TRUE;
    {
        std::lock_guard<std::mutex> lock(hall_connection_mutex_);
        bResult = hall_connection_->SendRequest(&Context, &Request, &Response, bTimeOut, wait_ms);
    }

    if (!bResult)///if timeout or disconnect 
    {
        UWL_ERR("SendHallRequest m_ConnHall->SendRequest fail bTimeOut = %d, nReqId = %d", bTimeOut, nReqId);
        assert(false);
        return std::make_tuple(false, (bTimeOut ? ERR_SENDREQUEST_TIMEOUT : ERR_OPERATE_FAILED));
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
        return std::make_tuple(false, info);
    }
    return std::make_tuple(true, ERR_OPERATE_SUCESS);
}

bool CRobotMgr::GetRobotSetting(int account, RobotSetting& unit) {
    if (account == 0)	return false;

    auto it = account_setting_map_.find(account);
    if (it == account_setting_map_.end()) return false;

    unit = it->second;

    return true;
}

RobotPtr CRobotMgr::GetRobotByToken(const EConnType& type, const TokenID& id) {
    std::lock_guard<std::mutex> lock(robot_map_mutex_);
    auto it = std::find_if(robot_map_.begin(), robot_map_.end(), [&] (const std::pair<UserID, RobotPtr>& it) {
        return it.second->GameToken() == id;
    });

    return it != robot_map_.end() ? it->second : nullptr;
}

void	CRobotMgr::ThreadRunHallNotify() {
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

void	CRobotMgr::ThreadRunGameNotify() {
    UWL_INF(_T("GameNotify thread started. id = %d"), GetCurrentThreadId());

    MSG msg = {};
    while (GetMessage(&msg, 0, 0, 0)) {
        if (UM_DATA_RECEIVED == msg.message) {

            LPCONTEXT_HEAD	pContext = (LPCONTEXT_HEAD) (msg.wParam);
            LPREQUEST		pRequest = (LPREQUEST) (msg.lParam);

            TokenID		nTokenID = pContext->lTokenID;
            RequestID		nReqstID = pRequest->head.nRequest;

            auto client = GetRobotByToken(ECT_GAME, nTokenID);
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

void	CRobotMgr::ThreadRunGameInfoNotify() {
    UWL_INF(_T("GameNotify thread started. id = %d"), GetCurrentThreadId());

    MSG msg = {};
    while (GetMessage(&msg, 0, 0, 0)) {
        if (UM_DATA_RECEIVED == msg.message) {

            LPCONTEXT_HEAD	pContext = (LPCONTEXT_HEAD) (msg.wParam);
            LPREQUEST		pRequest = (LPREQUEST) (msg.lParam);

            RequestID		nReqstID = pRequest->head.nRequest;

            OnGameInfoNotify(nReqstID, *pRequest);

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

void CRobotMgr::OnGameInfoNotify(RequestID nReqstID, const REQUEST &request) {
    switch (nReqstID) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
            OnDisconnGameInfo();
            break;

        case GN_USER_STATUS_TO_ROBOTSVR:
        {
            OnRecvGameInfo(request);
        }
        break;
    }

}

void CRobotMgr::OnRecvGameInfo(const REQUEST &request) {
    game::base::Status2RobotSvrNotify user_status_ntf;
    int parse_ret = ParseFromRequest(request, user_status_ntf);
    if (kCommSucc != parse_ret) {
        UWL_WRN("ParseFromRequest failed.");
        return;
    }

    UserID userid = user_status_ntf.userid();
    if (user_map_.find(userid) == user_map_.end()) return;

    auto user = user_map_[userid];
    user.set_userid(user_status_ntf.userid());
    user.set_roomid(user_status_ntf.roomid());
    user.set_tableno(user_status_ntf.tableno());
    user.set_chairno(user_status_ntf.chairno());
    user.set_user_type(user_status_ntf.user_type());

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
            //OnThrndDelyEnterGame(time(nullptr));
        }
    }
    UWL_INF(_T("EnterGame thread exiting. id = %d"), GetCurrentThreadId());
    return;
}

void CRobotMgr::OnHallNotify(RequestID nReqId, void* pDataPtr, int32_t nSize) {
    switch (nReqId) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
            OnDisconnHallWithLock(nReqId, pDataPtr, nSize);
            break;

    }
}
void CRobotMgr::OnGameNotify(RobotPtr client, RequestID nReqId, void* pDataPtr, int32_t nSize) {
    switch (nReqId) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
            OnDisconnGameWithLock(client, nReqId, pDataPtr, nSize);
            break;

    }
}

void CRobotMgr::OnDisconnHallWithLock(RequestID nReqId, void* pDataPtr, int32_t nSize) {
    UWL_ERR(_T("与大厅服务断开连接"));
    assert(false);
    hall_connection_->DestroyEx();
    {
        for (auto&& it = robot_map_.begin(); it != robot_map_.end(); it++) {
            it->second->SetLogon(false);
        }
    }
}

void CRobotMgr::OnDisconnGameWithLock(RobotPtr client, RequestID nReqId, void* pDataPtr, int32_t nSize) {
    UWL_WRN("userid = %d disconnect game ", client->GetUserID());
    client->OnDisconnGame();
}

void CRobotMgr::OnDisconnGameInfo() {
    game_info_connection_->DestroyEx();
}

TTueRet CRobotMgr::RobotLogonHall(const int32_t& account) {

    AccountSettingMap::iterator itRobot = account_setting_map_.end();

    //配置账号是否存在
    // 指定账号 否则随机
    if (account != 0) {
        auto&& it = account_setting_map_.find(account); // robot setting
        if (it == account_setting_map_.end())
            return std::make_tuple(false, "使用无效的机器人账号");

        if (IsLogon(account))
            return std::make_tuple(true, "不可重复登录");

        itRobot = it;
    } else {
        //random
        for (int i = 0; i < 10; i++) {
            auto it = account_setting_map_.begin();
            auto randnum = rand();
            auto mapSize = account_setting_map_.size();
            auto num = randnum % mapSize;
            std::advance(it, num);
            if (IsLogon(it->first)) continue;
            itRobot = it; break;
        }

    }

    if (itRobot == account_setting_map_.end()) {

        if (account == 0) {
            //UWL_DBG("没有剩余机器人账号数据可用于登陆大厅");
        } else {
            UWL_ERR("没有找到机器人账号数据 account = %d", account);
            assert(false);
        }
        return std::make_tuple(true, "没有找到机器人账号数据可用于登陆大厅");
    }

    //账号对应client是否已经生成, 是否已经登入大厅
    auto client = GetRobotClient(itRobot->first);
    if (!client)
        client = std::make_shared<Robot>(itRobot->second);

    if (client->IsLogon())
        return std::make_tuple(false, "已经登录");

    LOGON_USER_V2  logonUser = {};
    logonUser.nUserID = client->GetUserID();
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
    strcpy_s(logonUser.szPassword, client->Password().c_str());

    RequestID nResponse;
    //LPVOID	 pRetData = NULL;
    std::shared_ptr<void> pRetData;
    uint32_t nDataLen = sizeof(logonUser);
    auto it = SendHallRequest(GR_LOGON_USER_V2, nDataLen, &logonUser, nResponse, pRetData);
    if (!TUPLE_ELE(it, 0)) {
        UWL_ERR("ACCOUNT = %d GR_LOGON_USER_V2 FAIL", client->GetUserID());
        return it;
    }

    if (!(nResponse == GR_LOGON_SUCCEEDED || nResponse == GR_LOGON_SUCCEEDED_V2)) {
        UWL_ERR("ACCOUNT = %d GR_LOGON_USER_V2 FAIL", client->GetUserID());
        return std::make_tuple(false, std::to_string(nResponse));
    }

    client->SetLogon(true);
    client->SetLogonData((LPLOGON_SUCCEED_V2) pRetData.get());
    SetRobotClient(client);

    UWL_INF("account:%d userid:%d logon hall ok.", client->GetUserID(), client->GetUserID());
    return std::make_tuple(true, ERR_OPERATE_SUCESS);
}


TTueRet CRobotMgr::RobotGainDeposit(RobotPtr client) {
    static TCHAR szHttpUrl[128];
    static int nResult = GetPrivateProfileString(_T("RobotReward"), _T("HttpUrl_G"), _T("null"), szHttpUrl, sizeof(szHttpUrl), g_szIniFile);
    if (0 == strcmp(szHttpUrl, "null"))
        return std::make_tuple(false, "RobotReward HttpUrl_G ini read faild");

    TCHAR szField[128] = {};
    sprintf_s(szField, "ActiveID_Game%d", g_gameID);
    TCHAR szValue[128] = {};
    GetPrivateProfileString(_T("RobotReward"), szField, _T("null"), szValue, sizeof(szValue), g_szIniFile);
    if (0 == strcmp(szValue, "null"))
        return std::make_tuple(false, "RobotGainDeposit ActiveId ini read faild");

    Json::Value root;
    Json::FastWriter fast_writer;
    root["GameId"] = Json::Value(g_gameID);
    root["ActId"] = Json::Value(szValue);
    root["UserId"] = Json::Value(client->GetUserID());
    root["UserNickName"] = Json::Value("Robot");
    root["Num"] = Json::Value(client->m_nGainAmount);
    root["Operation"] = Json::Value(1);
    root["Device"] = Json::Value(xyConvertIntToStr(g_gameID) + "+" + xyConvertIntToStr(client->GetUserID()));
    root["Time"] = Json::Value((LONGLONG) time(NULL) * 1000);
    root["Sign"] = Json::Value(MD5String((root["ActId"].asString() + "|" + root["UserId"].asString() + "|" + root["Time"].asString() + "|" + ROBOT_APPLY_DEPOSIT_KEY).c_str()));

    CString strParam = fast_writer.write(root).c_str();
    CString strResult = ExecHttpRequestPost(szHttpUrl, strParam);
    Json::Reader reader;
    Json::Value _root;
    if (!reader.parse((LPCTSTR) strResult, _root))
        return std::make_tuple(false, "RobotGainDeposit Http result parse faild");
    if (_root["Code"].asInt() != 0) {
        UWL_ERR("m_Account = %d gain deposit fail, code = %d, strResult = %s", client->GetUserID(), _root["Code"].asInt(), strResult);
        //assert(false);
        return std::make_tuple(false, "RobotGainDeposit Http result Status false");
    }

    return std::make_tuple(true, ERR_OPERATE_SUCESS);
}

TTueRet CRobotMgr::RobotBackDeposit(RobotPtr client) {
    static TCHAR szHttpUrl[128];
    static int nResult = GetPrivateProfileString(_T("RobotReward"), _T("HttpUrl_C"), _T("null"), szHttpUrl, sizeof(szHttpUrl), g_szIniFile);
    if (0 == strcmp(szHttpUrl, "null"))
        return std::make_tuple(false, "RobotReward HttpUrl_C ini read faild");

    TCHAR szField[128] = {};
    sprintf_s(szField, "ActiveID_Game%d", g_gameID);
    TCHAR szValue[128] = {};
    GetPrivateProfileString(_T("RobotReward"), szField, _T("null"), szValue, sizeof(szValue), g_szIniFile);
    if (0 == strcmp(szValue, "null"))
        return std::make_tuple(false, "RobotBackDeposit ActiveId ini read faild");

    Json::Value root;
    Json::FastWriter fast_writer;
    root["GameId"] = Json::Value(g_gameID);
    root["ActId"] = Json::Value(szValue);
    root["UserId"] = Json::Value(client->GetUserID());
    root["UserNickName"] = Json::Value("Robot");
    root["Num"] = Json::Value(client->m_nBackAmount);
    root["Operation"] = Json::Value(2);
    root["Device"] = Json::Value(xyConvertIntToStr(g_gameID) + "+" + xyConvertIntToStr(client->GetUserID()));
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
    auto count = 0;
    return count;
}


int CRobotMgr::GetUserStatus(UserID userid, UserStatus& user_status) {
    std::lock_guard<std::mutex> lock(game_info_connection_mutex_);

    if (user_map_.find(userid) == user_map_.end()) return kCommFaild;
    auto user = user_map_[userid];
    auto chairno = user.chairno();

    // 玩家信息中token为0则说明玩家离线； kUserOffline = 0x10000000		// 断线
    // TODO

    // 玩家信息中椅子号为0则说明在旁观；
    if (chairno == 0) {
        user_status = kUserLooking;
        return kCommSucc;
    }

    // 有椅子号则查看桌子状态，桌子waiting -> 玩家waiting
    game::base::Table table;
    if (kCommFaild == FindTable(userid, table)) return kCommFaild;
    auto table_status = table.table_status();
    if (table_status == kTableWaiting) {
        user_status = kUserWaiting;
        return kCommSucc;
    }

    // 桌子playing && 椅子playing -> 玩家playing
    game::base::ChairInfo chair;
    if (kCommFaild == FindChair(userid, chair)) return kCommFaild;
    auto chair_status = chair.chair_status();
    if (table_status != kTablePlaying) return kCommFaild;

    if (chair_status == kChairPlaying) {
        user_status = kUserPlaying;
        return kCommSucc;
    }

    // 桌子playing && 椅子waiting -> 等待下局游戏开始（原空闲玩家）//TODO 原空闲玩家?
    if (chair_status == kChairWaiting) {
        user_status = kUserWaiting;
        return kCommSucc;
    }

    return kCommFaild;
}

void    CRobotMgr::OnServerMainTimer(time_t nCurrTime) {
    OnTimerLogonHall(nCurrTime);
    /*   OnTimerSendHallPluse(nCurrTime);
       OnTimerSendGamePluse(nCurrTime);*/
    OnTimerUpdateDeposit(nCurrTime);
}
bool	CRobotMgr::OnTimerLogonHall(time_t nCurrTime) {
    if (!hall_connection_) {
        UWL_ERR("SendHallRequest OnTimerReconnectHall nil ERR_CONNECT_NOT_EXIST");
        assert(false);
        return false;
    }


    if (hall_connection_->IsConnected()) return true;

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
        for (auto&& it = account_setting_map_.begin(); it != account_setting_map_.end(); it++) {
            if (!IsLogon(it->first)) continue;

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
//void    CRobotMgr::OnTimerSendHallPluse(time_t nCurrTime) {
//    if (!hall_connection_) {
//        UWL_ERR("SendHallRequest OnTimerSendHallPluse nil ERR_CONNECT_NOT_EXIST nReqId");
//        assert(false);
//        return;
//    }
//
//
//    if (!hall_connection_->IsConnected()) {
//        UWL_ERR("SendHallRequest OnTimerSendHallPluse not connect ERR_CONNECT_DISABLE nReqId");
//        assert(false);
//        return;
//    }
//
//
//#define MAIN_SEND_HALL_PULSE_GAP_TIME (1*60) // 1分钟
//    static time_t	sLastSendHPulseGapTime = nCurrTime;
//    if (nCurrTime - sLastSendHPulseGapTime >= MAIN_SEND_HALL_PULSE_GAP_TIME)
//        sLastSendHPulseGapTime = nCurrTime;
//    else return;
//    {
//        HALLUSER_PULSE hp = {};
//        hp.nUserID = 0;
//        hp.nAgentGroupID = ROBOT_AGENT_GROUP_ID;
//
//        RequestID nResponse;
//        //LPVOID	 pRetData = NULL;
//        std::shared_ptr<void> pRetData;
//        uint32_t nDataLen = sizeof(HALLUSER_PULSE);
//        (void) SendHallRequest(GR_HALLUSER_PULSE, nDataLen, &hp, nResponse, pRetData, false);
//    }
//}
//
//void    CRobotMgr::OnTimerSendGamePluse(time_t nCurrTime) {
//#define MAIN_SEND_GAME_PULSE_GAP_TIME (1*60+2) // 1分钟2秒
//    static time_t	sLastSendGPulseGapTime = nCurrTime;
//    if (nCurrTime - sLastSendGPulseGapTime >= MAIN_SEND_GAME_PULSE_GAP_TIME)
//        sLastSendGPulseGapTime = nCurrTime;
//    else return;
//    {
//        std::lock_guard<std::mutex> lock(robot_map_mutex_);
//        for (auto&& it = robot_map_.begin(); it != robot_map_.end(); it++) {
//            if (!it->second->IsGaming())	continue;
//
//            it->second->SendGamePulse();
//        }
//    }
//}

void    CRobotMgr::OnTimerUpdateDeposit(time_t nCurrTime) {
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

#define UPDATE_CLIENTNUM_PRE_ONCE	(15)
#define MAIN_ROOM_UPDATE_DEPOSIT_GAP_TIME (15) // 15s
    static time_t	sLastRoomUpdateDepositGapTime = 0;
    if (nCurrTime - sLastRoomUpdateDepositGapTime >= MAIN_ROOM_UPDATE_DEPOSIT_GAP_TIME)
        sLastRoomUpdateDepositGapTime = nCurrTime;
    else return;

    int nCount = 0;
    {
        std::lock_guard<std::mutex> lock(robot_map_mutex_);
        for (auto&& it = robot_map_.begin(); it != robot_map_.end(); it++) {
            if (!it->second->IsLogon())	continue;

            //if (it->second->GetRoomID() != 0)	continue;

            //if (it->second->IsGaming())		continue;

            if (it->second->m_bGainDeposit) {
                it->second->m_bGainDeposit = false;
                auto&& tRet = RobotGainDeposit(it->second);
                if (!TUPLE_ELE(tRet, 0)) {
                    UWL_WRN("Account:%d GainDeposit Err:%s", it->second->GetUserID(), TUPLE_ELE_C(tRet, 1));
                    continue;
                }
                nCount++;
            }
            if (it->second->m_bBackDeposit) {
                it->second->m_bBackDeposit = false;
                auto&& tRet = RobotBackDeposit(it->second);
                if (!TUPLE_ELE(tRet, 0)) {
                    UWL_WRN("Account:%d BackDeposit Err:%s", it->second->GetUserID(), TUPLE_ELE_C(tRet, 1));
                    continue;
                }
                nCount++;
            }
            if (nCount >= UPDATE_CLIENTNUM_PRE_ONCE)
                break;
        }
    }
}
//void    CRobotMgr::OnThrndDelyEnterGame(time_t nCurrTime) {
//    do {
//        CRobotClient* pRoboter = GetWaitEnter();
//        if (pRoboter == nullptr)	return;
//
//        ROOM theRoom = {};
//        if (!GetRoomData(pRoboter->RoomId(), theRoom)) {
//            UWL_ERR("Room %s: client[%d] room[%d] not found.", pRoboter->m_sEnterWay.c_str(), pRoboter->GetUserID(), pRoboter->RoomId());
//            //assert(false);
//            continue;
//        }
//        stRobotUnit unit;
//        if (!GetRobotSetting(pRoboter->GetUserID(), unit)) {
//            UWL_ERR("Room %s: client[%d] RobotUnit not found.", pRoboter->m_sEnterWay.c_str(), pRoboter->GetUserID());
//            assert(false);
//            continue;
//        }
//
//        auto begTime = GetTickCount();
//        UWL_DBG("[PROFILE] 0 SendEnterGame timestamp = %ld", GetTickCount());
//        auto ret = pRoboter->SendEnterGame(theRoom, m_thrdGameNotify.ThreadId(), unit.nickName, unit.portraitUrl, pRoboter->m_nToTable, pRoboter->m_nToChair);
//        UWL_DBG("[PROFILE] 7 SendEnterGame timestamp = %ld", GetTickCount());
//        if (!TUPLE_ELE(ret, 0)) {
//            UWL_WRN("Room %s: client[%d] enter game faild:%s.", pRoboter->m_sEnterWay.c_str(), pRoboter->GetUserID(), TUPLE_ELE_C(ret, 1));
//            UWL_WRN("Room %s: client[%d] enter game faild try to reset the socket", pRoboter->m_sEnterWay.c_str(), pRoboter->GetUserID());
//            UWL_DBG("[interval] OnThrndDelyEnterGame client[%d] end fail, time = %I32u", pRoboter->GetUserID(), time(nullptr));
//            OnCliDisconnGame(pRoboter, 0, nullptr, 0);
//            continue;
//        }
//        auto endTime = GetTickCount();
//        UWL_DBG("[interval] Room %s: client[%d] Dely Enter Game OK end successfully. time %d ", pRoboter->m_sEnterWay.c_str(), pRoboter->GetUserID(), time(nullptr));
//        UWL_DBG("[interval] [PROFILE] ENTER GAME cost time = %ld", endTime - begTime);
//
//        UWL_INF("[STATUS] client = %d status waitting", pRoboter->GetUserID());
//
//
//    } while (true);
//}


int CRobotMgr::FindTable(UserID userid, game::base::Table& table) {
    if (user_map_.find(userid) == user_map_.end()) return kCommFaild;
    auto user = user_map_[userid];
    auto tableno = user.tableno();

    auto roomid = user.roomid();
    if (room_map_.find(roomid) == room_map_.end()) return kCommFaild;

    auto tables = room_map_[roomid].tables();

    for (auto& table : tables) {
        if (table.tableno() == tableno) {
            table = tables[tableno];
            return kCommSucc;
        }
    }
    return kCommFaild;
}


int CRobotMgr::FindChair(UserID userid, game::base::ChairInfo& chair) {
    if (user_map_.find(userid) == user_map_.end()) return kCommFaild;
    auto user = user_map_[userid];
    auto chairno = user.chairno();

    game::base::Table table;
    if (kCommFaild == FindTable(userid, table)) return kCommFaild;

    auto chairs = table.chairs();

    for (auto& chair : chairs) {
        if (chair.chairno() == chairno) {
            chair = chairs[chairno];
            return kCommSucc;
        }
    }

    return kCommFaild;
}