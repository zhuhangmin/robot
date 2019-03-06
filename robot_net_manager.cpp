#include "stdafx.h"
#include "robot_net_manager.h"
#include "main.h"
#include "robot_utils.h"
#include "robot_define.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


int RobotNetManager::Init() {
    LOG_FUNC("[START ROUTINE]");
    heart_thread_.Initial(std::thread([this] {this->ThreadRobotPulse(); }));
    notify_thread_.Initial(std::thread([this] {this->ThreadRobotNotify(); }));
    LOG_INFO("RobotGameManager::Init Sucessed.");
    return kCommSucc;
}

int RobotNetManager::Term() {
    heart_thread_.Release();
    notify_thread_.Release();
    LOG_FUNC("[EXIT ROUTINE]");
    return kCommSucc;
}

int RobotNetManager::GetRobotWithCreate(const UserID& userid, RobotPtr& robot) {
    CHECK_USERID(userid);
    std::lock_guard<std::mutex> lock(robot_map_mutex_);
    if (kCommSucc != GetRobotWithLock(userid, robot)) {
        robot = std::make_shared<RobotNet>(userid);
        SetRobotWithLock(robot);
    }

    if (kCommSucc != GetRobotWithLock(userid, robot)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

ThreadID RobotNetManager::GetRobotNotifyThreadID() const {
    std::lock_guard<std::mutex> lock(robot_map_mutex_);
    return notify_thread_.GetThreadID();
}

// 具体业务

// 让500个机器人均匀的发心跳 避免瞬时500个心跳触发获得业务锁，造成消息堆积
int RobotNetManager::SendGamePulse() {
    std::lock_guard<std::mutex> lock(robot_map_mutex_);
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
        return pair.second->GetTokenID() == token_id;
    });
    if (it != robot_map_.end()) {
        robot = it->second;
    } else {
        robot = nullptr;
    }
    return kCommSucc;
}

int RobotNetManager::ThreadRobotPulse() {
    LOG_INFO("[START ROUTINE] RobotGameManager KeepAlive thread [%d] started", GetCurrentThreadId());
    while (true) {
        const auto dwRet = WaitForSingleObject(g_hExitServer, RobotTimerInterval);
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }

        if (WAIT_TIMEOUT == dwRet) {
            SendGamePulse();
        }
    }
    LOG_INFO("[EXIT ROUTINE] RobotGameManager KeepAlive thread [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}

int RobotNetManager::ThreadRobotNotify() {
    LOG_INFO("[START ROUTINE] RobotGameManager Notify thread [%d] started", GetCurrentThreadId());

    MSG msg = {};
    while (GetMessage(&msg, 0, 0, 0)) {
        if (UM_DATA_RECEIVED == msg.message) {

            auto pContext = reinterpret_cast<LPCONTEXT_HEAD>(msg.wParam);
            auto pRequest = reinterpret_cast<LPREQUEST>(msg.lParam);
            const auto nTokenID = pContext->lTokenID;
            const auto requestid = pRequest->head.nRequest;

            OnRobotNotify(requestid, pRequest->pDataPtr, pRequest->nDataLen, nTokenID);

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

int RobotNetManager::OnRobotNotify(const RequestID& requestid, void* ntf_data_ptr, const int& data_size, const TokenID& token_id) const {
    RobotPtr robot;
    {
        std::lock_guard<std::mutex> lock(robot_map_mutex_);
        if (kCommSucc != GetRobotByTokenWithLock(token_id, robot))
            return kCommFaild;
    }

    switch (requestid) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
            robot->OnDisconnGame();
            break;
        default:
            break;
    }
    return kCommSucc;
}

int RobotNetManager::SnapShotObjectStatus() {
    std::lock_guard<std::mutex> lock(robot_map_mutex_);
    LOG_INFO("OBJECT ADDRESS [%x]", this);
    LOG_INFO("robot_notify_thread_ [%d]", notify_thread_.GetThreadID());
    LOG_INFO("robot_heart_timer_thread_ [%d]", heart_thread_.GetThreadID());

    LOG_INFO("robot_map_ size [%d]", robot_map_.size());
    std::string str = "{";
    for (auto& kv : robot_map_) {
        const auto userid = kv.first;
        const auto robot = kv.second;
        const auto token = robot->GetTokenID();
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


int RobotNetManager::CheckNotInnerThread() {
    CHECK_NOT_THREAD(notify_thread_);
    CHECK_NOT_THREAD(heart_thread_);
    return kCommSucc;
}