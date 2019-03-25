#include "stdafx.h"
#include "robot_net_manager.h"
#include "main.h"
#include "robot_utils.h"
#include "robot_define.h"
#include "game_net_manager.h"
#include "deposit_data_manager.h"
#include "common_func.h"

int RobotNetManager::Init() {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO_FUNC("\t[START]");
    timer_thread_.Initial(std::thread([this] {this->ThreadTimer(); }));
    notify_thread_.Initial(std::thread([this] {this->ThreadNotify(); }));
    return kCommSucc;
}

int RobotNetManager::Term() {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    timer_thread_.Release();
    notify_thread_.Release();
    robot_map_.clear();

    LOG_INFO_FUNC("[EXIT]");
    return kCommSucc;
}

int RobotNetManager::EnterGame(const UserID& userid, const RoomID& roomid, const TableNO& tableno, const std::string& game_ip, const int& game_port) {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_USERID(userid);

    //获得robot,无则生成.一旦生成常驻内存
    RobotPtr robot;
    if (kCommSucc != GetRobotWithLock(userid, robot)) {
        robot = std::make_shared<RobotNet>(userid);
        SetRobotWithLock(robot);
    }
    if (!robot) ASSERT_FALSE_RETURN;

    // @zhuhangmin 20190223 issue: 网络库不支持域名IPV6解析，使用配置IP
    // 建立连接
    BOOL is_connected = false;
    if (kCommSucc != robot->IsConnected(is_connected)) {
        ASSERT_FALSE_RETURN;
    }
    if (!is_connected) {
        LOG_ROUTE("connect game", roomid, tableno, userid);
        if (kCommSucc != robot->Connect(game_ip, game_port, notify_thread_.GetThreadID())) {
            ASSERT_FALSE_RETURN;
        }
    }
    return robot->SendEnterGame(roomid, tableno);
}

// 具体业务
int RobotNetManager::KeepConnection() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& kv : robot_map_) {
        auto robot = kv.second;
        if (!robot) {
            ASSERT_FALSE;
            continue;
        }

#ifdef CURRENT_DELAY
        //"正式版防止大量消息冲击后端服务器 [副作用] 正式版机器人启动较慢"
        SLEEP_FOR(CurrentDealy);
#endif

        robot->KeepConnection();
    }
    return kCommSucc;
}

int RobotNetManager::ResultOneUserWithLock(const UserID& robot_userid, const REQUEST &request) const {
    game::base::GameResultNotify ntf;
    const auto parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        LOG_WARN("ParseFromRequest failed.");
        ASSERT_FALSE_RETURN;
    }

    // 更新用户
    const auto& user_results = ntf.user_results();
    const auto size = ntf.user_results_size();
    for (auto result_index = 0; result_index < size; result_index++) {
        const auto& result = user_results[result_index];
        const auto userid = result.userid();
        const auto chairno = result.chairno();
        const int64_t old_deposit = result.old_deposit();
        const int64_t diff_deposit = result.diff_deposit();
        const auto fee = result.fee();
        CHECK_DEPOSIT(old_deposit);
        int64_t cur_deposit = old_deposit + diff_deposit;
        CHECK_DEPOSIT(cur_deposit);
        if (robot_userid == userid) {
            LOG_INFO("ResultOneUser userid [%d] old_deposit [%I64d] diff_deposit [%I64d]",
                     userid, old_deposit, diff_deposit);
            if (kCommSucc != DepositDataMgr.SetDeposit(userid, cur_deposit)) {
                ASSERT_FALSE_RETURN;
            }
        }
    }
    return kCommSucc;
}

int RobotNetManager::ResultTableWithLock(const UserID& robot_userid, const REQUEST &request) const {
    game::base::GameResultNotify ntf;
    const auto parse_ret = ParseFromRequest(request, ntf);
    if (kCommSucc != parse_ret) {
        LOG_WARN("ParseFromRequest failed.");
        ASSERT_FALSE_RETURN;
    }

    // 更新用户
    const auto& user_results = ntf.user_results();
    const auto size = ntf.user_results_size();
    for (auto result_index = 0; result_index < size; result_index++) {
        const auto& result = user_results[result_index];
        const auto userid = result.userid();
        const auto chairno = result.chairno();
        const int64_t old_deposit = result.old_deposit();
        const int64_t diff_deposit = result.diff_deposit();
        const auto fee = result.fee();
        CHECK_DEPOSIT(old_deposit);
        const int64_t cur_deposit = old_deposit + diff_deposit;
        CHECK_DEPOSIT(cur_deposit);
        if (robot_userid == userid) {
            LOG_INFO("[DEPOSIT] ResultTable userid [%d] old_deposit [%I64d] diff_deposit [%I64d]",
                     userid, old_deposit, diff_deposit);
            if (kCommSucc != DepositDataMgr.SetDeposit(userid, cur_deposit)) {
                ASSERT_FALSE_RETURN;
            }
        }
    }
    return kCommSucc;
}

int RobotNetManager::GetRobotWithLock(const UserID& userid, RobotPtr& robot) const {
    CHECK_USERID(userid);
    const auto& iter = robot_map_.find(userid);
    if (iter == robot_map_.end()) {
        return kCommFaild;
    }
    robot = iter->second;
    return kCommSucc;
}

int RobotNetManager::SetRobotWithLock(const RobotPtr& robot) {
    if (!robot) return kCommFaild;
    robot_map_.insert(std::make_pair(robot->GetUserID(), robot));
    return kCommSucc;
}

int RobotNetManager::GetRobotByTokenWithLock(const TokenID& token_id, RobotPtr& robot) const {
    CHECK_TOKENID(token_id);
    const auto it = std::find_if(robot_map_.begin(), robot_map_.end(), [&] (const std::pair<UserID, RobotPtr>& pair) {
        auto token = InvalidTokenID;
        if (kCommSucc != pair.second->GetTokenID(token)) {
            return false;
        }
        return  token == token_id;

    });
    if (it != robot_map_.end()) {
        robot = it->second;
    } else {
        robot = nullptr;
    }
    return kCommSucc;
}

int RobotNetManager::ThreadTimer() {
    LOG_INFO("\t[START] timer thread [%d] started", GetCurrentThreadId());
    while (true) {
        const auto dwRet = WaitForSingleObject(g_hExitServer, TimerInterval);
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }

        if (WAIT_TIMEOUT == dwRet) {
            if (!g_inited) continue;
            if (!GameMgr.IsGameDataInited()) { // 脏读
                LOG_WARN("game data has not inited yet");
                continue;
            }
            KeepConnection();
        }
    }
    LOG_INFO("[EXIT] timer thread  [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}

int RobotNetManager::ThreadNotify() {
    LOG_INFO("\t[START] robot notify thread [%d] started", GetCurrentThreadId());

    MSG msg = {};
    while (GetMessage(&msg, 0, 0, 0)) {
        if (UM_DATA_RECEIVED == msg.message) {

            auto pContext = reinterpret_cast<LPCONTEXT_HEAD>(msg.wParam);
            auto pRequest = reinterpret_cast<LPREQUEST>(msg.lParam);
            const auto requestid = pRequest->head.nRequest;
            const auto nTokenID = pContext->lTokenID;

            OnNotify(requestid, *pRequest, nTokenID);

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

int RobotNetManager::OnNotify(const RequestID& requestid, const REQUEST& request, const TokenID& token_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    RobotPtr robot;
    if (kCommSucc != GetRobotByTokenWithLock(token_id, robot)) ASSERT_FALSE_RETURN;
    if (!robot) ASSERT_FALSE_RETURN;

    const auto robot_userid = robot->GetUserID();

    if (UR_SOCKET_ERROR == requestid ||
        UR_SOCKET_CLOSE == requestid ||
        GN_GAME_RESULT_TABLE == requestid ||
        GN_GAME_RESULT_ONEUSER == requestid||
        GN_USER_DEPOSIT_CHANGE == requestid) {
        auto token = InvalidTokenID;
        if (kCommSucc == robot->GetTokenID(token)) {
            LOG_INFO("robot userid [%d] token [%d] [RECV] requestid [%d] [%s]", robot_userid, token, requestid, REQ_STR(requestid));
        }
    }

    if (UR_SOCKET_ERROR != requestid &&
        UR_SOCKET_CLOSE != requestid) {
        if (!GameMgr.IsGameDataInited()) {// 脏读
            const auto userid = robot->GetUserID();
            LOG_WARN("game data has inited yet, [DISCARD] robot userid [%d] requestid [%d] [%s] kExceptionGameDataNotInited",
                     userid, requestid, REQ_STR(requestid));
            return kExceptionGameDataNotInited;
        }
    }

    switch (requestid) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
            robot->OnDisconnect();
            break;
        case GN_GAME_RESULT_TABLE:
            ResultTableWithLock(robot_userid, request);
            break;
        case GN_GAME_RESULT_ONEUSER:
            ResultOneUserWithLock(robot_userid, request);
            break;
        case GN_USER_DEPOSIT_CHANGE:
            LOG_WARN("NO CASE TO TEST YET, PLZ FIX ME! [GN_USER_DEPOSIT_CHANGE]");
            break;
        default:
            break;
    }
    return kCommSucc;
}

int RobotNetManager::SnapShotObjectStatus() {
    CHECK_MAIN_OR_LAUNCH_THREAD();
#ifdef _DEBUG
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("robot_notify_thread_ [%d]", notify_thread_.GetThreadID());
    LOG_INFO("robot_heart_timer_thread_ [%d]", timer_thread_.GetThreadID());

    auto robot_in_game = 0;
    LOG_INFO("robot_map_ size [%d]", robot_map_.size());
    std::string str = "{";
    for (auto& kv : robot_map_) {
        const auto userid = kv.first;
        const auto robot = kv.second;
        auto token = InvalidTokenID;
        if (kCommSucc != robot->GetTokenID(token))  continue;
        str += "userid";
        str += "[";
        str += std::to_string(userid);
        str += "]";
        str += "token";
        str += "[";
        str += std::to_string(token);
        str += "], ";
    }
    str += "}";

    LOG_INFO("all robot [%s]", str.c_str());
#endif
    return kCommSucc;
}

int RobotNetManager::BriefInfo() const {
#ifdef _DEBUG
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("robot_map_ size [%d]", robot_map_.size());
#endif
    return kCommSucc;
}
