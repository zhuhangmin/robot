#include "stdafx.h"
#include "robot_net_manager.h"
#include "main.h"
#include "robot_utils.h"
#include "robot_define.h"
#include "user_manager.h"

int RobotNetManager::Init() {
    LOG_INFO_FUNC("\t[START]");
    timer_thread_.Initial(std::thread([this] {this->ThreadTimer(); }));
    notify_thread_.Initial(std::thread([this] {this->ThreadNotify(); }));
    return kCommSucc;
}

int RobotNetManager::Term() {
    timer_thread_.Release();
    notify_thread_.Release();
    robot_map_.clear();

    LOG_INFO_FUNC("[EXIT]");
    return kCommSucc;
}

int RobotNetManager::EnterGame(const UserID& userid, const RoomID& roomid, const TableNO& tableno) {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_USERID(userid);

    //获得robot,无则生成.一旦生成常驻内存
    RobotPtr robot;
    if (kCommSucc != GetRobotWithLock(userid, robot)) {
        robot = std::make_shared<RobotNet>(userid);
        SetRobotWithLock(robot);
    }
    if (!robot) return kCommFaild;

    // @zhuhangmin 20190223 issue: 网络库不支持域名IPV6解析，使用配置IP
    // 建立连接
    BOOL is_connected = false;
    if (kCommSucc != robot->IsConnected(is_connected)) {
        ASSERT_FALSE_RETURN;
    }
    if (!is_connected) {
        LOG_ROUTE("connect game", roomid, tableno, userid);
        const auto game_ip = RobotUtils::GetGameIP();
        const auto game_port = RobotUtils::GetGamePort();
        if (kCommSucc != robot->Connect(game_ip, game_port, notify_thread_.GetThreadID())) {
            ASSERT_FALSE_RETURN;
        }
    }

    // 发送进入游戏
    LOG_ROUTE("enter game", roomid, tableno, userid);
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
        if (timestamp - now > RobotPulseInterval) {
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
    //在游戏的机器人才需要重连
    for (auto& kv : robot_map_) {
        auto userid = kv.first;
        if (!UserMgr.IsRobotUserExist(userid)) continue;
        auto robot = kv.second;
        if (!robot) {
            ASSERT_FALSE;
            continue;
        }
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
    CHECK_ROBOT(robot);
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
            KeepConnection();
            SendPulse();
        }
    }
    LOG_INFO("[EXIT] timer thread  [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}

int RobotNetManager::ThreadNotify() const {
    LOG_INFO("\t[START] notify thread [%d] started", GetCurrentThreadId());

    MSG msg = {};
    while (GetMessage(&msg, 0, 0, 0)) {
        if (UM_DATA_RECEIVED == msg.message) {

            auto pContext = reinterpret_cast<LPCONTEXT_HEAD>(msg.wParam);
            auto pRequest = reinterpret_cast<LPREQUEST>(msg.lParam);
            const auto nTokenID = pContext->lTokenID;
            const auto requestid = pRequest->head.nRequest;

            OnNotify(requestid, pRequest->pDataPtr, pRequest->nDataLen, nTokenID);

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

int RobotNetManager::OnNotify(const RequestID& requestid, void* ntf_data_ptr, const int& data_size, const TokenID& token_id) const {
    RobotPtr robot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (kCommSucc != GetRobotByTokenWithLock(token_id, robot)) ASSERT_FALSE_RETURN;
    }
    if (!robot) return kCommFaild;

    switch (requestid) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
        {
            LOG_INFO("[RECV] [%d] [%s]", requestid, REQ_STR(requestid));

            auto token = InvalidTokenID;
            if (kCommSucc != robot->GetTokenID(token)) {
                LOG_INFO("robot token [%d] [RECV] requestid [%d] [%s]", token, requestid, REQ_STR(requestid));
            }

            robot->OnDisconnect();
        }
        break;
        default:
            break;
    }
    return kCommSucc;
}

int RobotNetManager::SnapShotObjectStatus() {
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

        if (UserMgr.IsRobotUserExist(userid)) {
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
    return kCommSucc;
}

int RobotNetManager::BriefInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("robot_map_ size [%d]", robot_map_.size());
    return kCommSucc;
}

int RobotNetManager::CheckNotInnerThread() {
    CHECK_NOT_THREAD(notify_thread_);
    CHECK_NOT_THREAD(timer_thread_);
    return kCommSucc;
}