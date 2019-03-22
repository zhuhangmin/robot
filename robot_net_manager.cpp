#include "stdafx.h"
#include "robot_net_manager.h"
#include "main.h"
#include "robot_utils.h"
#include "robot_define.h"
#include "user_manager.h"
#include "game_net_manager.h"

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

int RobotNetManager::EnterGame(const UserID& userid, const RoomID& roomid, const TableNO& tableno, std::string game_ip, int game_port) {
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

// 让500个机器人均匀的发心跳 避免瞬时500个心跳触发获得业务锁，造成消息堆积
int RobotNetManager::SendPulse() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& kv : robot_map_) {
        auto robot = kv.second;
        const auto timestamp = robot->GetTimeStamp();
        const auto now = std::time(nullptr);
        if (now - timestamp > RobotPulseInterval) {
            robot->SetTimeStamp(now);
            BOOL is_connected = false;
            if (kCommSucc != robot->IsConnected(is_connected)) {
                continue;
            }
            if (is_connected) {
                robot->SendPulse();
            }
        }

    }
    return kCommSucc;
}

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
            SendPulse();
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
            auto nTokenID = pContext->lTokenID;

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
    RobotPtr robot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (kCommSucc != GetRobotByTokenWithLock(token_id, robot)) ASSERT_FALSE_RETURN;
    }
    if (!robot) return kCommFaild;

    if (UR_SOCKET_ERROR == requestid ||
        UR_SOCKET_CLOSE == requestid ||
        GN_GAME_RESULT_TABLE == requestid ||
        GN_GAME_RESULT_ONEUSER == requestid||
        GN_USER_DEPOSIT_CHANGE == requestid) {
        auto token = InvalidTokenID;
        const auto userid = robot->GetUserID();
        if (kCommSucc == robot->GetTokenID(token)) {
            LOG_INFO("robot userid [%d] token [%d] [RECV] requestid [%d] [%s]", userid, token, requestid, REQ_STR(requestid));
        }
    }

    if (UR_SOCKET_ERROR != requestid &&
        UR_SOCKET_CLOSE != requestid) {
        if (!GameMgr.IsGameDataInited()) {
            const auto userid = robot->GetUserID();
            LOG_WARN("game data has inited yet robot userid [%d] requestid [%d] [%s] kExceptionGameDataNotInited", userid, requestid, REQ_STR(requestid));
            return kExceptionGameDataNotInited;
        }
    }

    switch (requestid) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
            robot->OnDisconnect();
            break;
        case GN_GAME_RESULT_TABLE:
            robot->ResultTable(request);
            break;
        case GN_GAME_RESULT_ONEUSER:
            robot->ResultOneUser(request);
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
    std::string in_game_str = "{";
    LOG_INFO("robot_map_ size [%d]", robot_map_.size());
    std::string str = "{";
    for (auto& kv : robot_map_) {
        const auto userid = kv.first;
        const auto robot = kv.second;
        auto token = InvalidTokenID;
        if (kCommSucc != robot->GetTokenID(token))  continue;

        if (kCommSucc == UserMgr.IsRobotUserExist(userid)) {
            robot_in_game++;
            in_game_str += "userid";
            in_game_str += "[";
            in_game_str += std::to_string(userid);
            in_game_str += "]";
            in_game_str += "token";
            in_game_str += "[";
            in_game_str += std::to_string(token);
            in_game_str += "], ";
        }

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
    in_game_str += "}";

    LOG_INFO("all robot [%s]", str.c_str());
    LOG_INFO("robot in game count [%d] [%s]", robot_in_game, in_game_str.c_str());
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
