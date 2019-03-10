#include "stdafx.h"
#include "robot_net_manager.h"
#include "main.h"
#include "robot_utils.h"
#include "robot_define.h"

int RobotNetManager::Init() {
    LOG_INFO_FUNC("[SVR START]");
    heart_thread_.Initial(std::thread([this] {this->ThreadTimer(); }));
    notify_thread_.Initial(std::thread([this] {this->ThreadNotify(); }));
    return kCommSucc;
}

int RobotNetManager::Term() {
    heart_thread_.Release();
    notify_thread_.Release();
    LOG_INFO_FUNC("[EXIT ROUTINE]");
    return kCommSucc;
}

int RobotNetManager::GetRobotWithCreate(const UserID& userid, RobotPtr& robot) {
    CHECK_USERID(userid);
    std::lock_guard<std::mutex> lock(mutex_);
    if (kCommSucc != GetRobotWithLock(userid, robot)) {
        robot = std::make_shared<RobotNet>(userid);
        SetRobotWithLock(robot);
    }

    if (kCommSucc != GetRobotWithLock(userid, robot)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

ThreadID RobotNetManager::GetNotifyThreadID() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return notify_thread_.GetThreadID();
}

// 具体业务

// 让500个机器人均匀的发心跳 避免瞬时500个心跳触发获得业务锁，造成消息堆积
int RobotNetManager::SendPulse() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& kv : robot_map_) {
        auto robot = kv.second;
        const auto timestamp = robot->GetTimeStamp();
        const auto now = time(0);
        if (timestamp - now > PulseInterval) {
            robot->SetTimeStamp(now);
            if (robot->IsConnected()) {
                robot->SendGamePulse();
            }
        }

    }
    return kCommSucc;
}

int RobotNetManager::KeepConnection() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& kv : robot_map_) {
        auto robot = kv.second;
        if (!robot) ASSERT_FALSE_RETURN;
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
        TokenID token = InvalidTokenID;
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
    LOG_INFO("[SVR START] RobotGameManager KeepAlive thread [%d] started", GetCurrentThreadId());
    while (true) {
        const auto dwRet = WaitForSingleObject(g_hExitServer, RobotTimerInterval);
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }

        if (WAIT_TIMEOUT == dwRet) {
            KeepConnection();
            SendPulse();
        }
    }
    LOG_INFO("[EXIT ROUTINE] RobotGameManager KeepAlive thread [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}

int RobotNetManager::ThreadNotify() {
    LOG_INFO("[SVR START] RobotGameManager Notify thread [%d] started", GetCurrentThreadId());

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
    LOG_INFO("[EXIT ROUTINE] RobotGameManager Notify thread [%d] exiting", GetCurrentThreadId());
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
            robot->OnDisconnect();
            break;
        default:
            break;
    }
    return kCommSucc;
}

int RobotNetManager::SnapShotObjectStatus() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("robot_notify_thread_ [%d]", notify_thread_.GetThreadID());
    LOG_INFO("robot_heart_timer_thread_ [%d]", heart_thread_.GetThreadID());

    LOG_INFO("robot_map_ size [%d]", robot_map_.size());
    std::string str = "{";
    for (auto& kv : robot_map_) {
        const auto userid = kv.first;
        const auto robot = kv.second;
        TokenID token = InvalidTokenID;
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

    LOG_INFO(" [%s]", str.c_str());
    return kCommSucc;
}

int RobotNetManager::BriefInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("robot_map_ size [%d]", robot_map_.size());
    return kCommSucc;
}

int RobotNetManager::CheckNotInnerThread() {
    CHECK_NOT_THREAD(notify_thread_);
    CHECK_NOT_THREAD(heart_thread_);
    return kCommSucc;
}