#include "stdafx.h"
#include "robot_game_manager.h"
#include "main.h"
#include "common_func.h"
#include "robot_utils.h"
#include "robot_define.h"
#include "setting_manager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


int RobotGameManager::Init() {
    robot_heart_timer_thread_.Initial(std::thread([this] {this->ThreadRobotPluse(); }));
    robot_notify_thread_.Initial(std::thread([this] {this->ThreadRobotNotify(); }));
    UWL_INF("RobotGameManager::Init Sucessed.");
    return kCommSucc;
}

int RobotGameManager::Term() {
    robot_heart_timer_thread_.Release();
    robot_notify_thread_.Release();
    return kCommSucc;
}

int RobotGameManager::GetRobotWithCreate(const UserID userid, RobotPtr& robot) {
    CHECK_USERID(userid);
    std::lock_guard<std::mutex> lock(robot_map_mutex_);
    if (kCommSucc != GetRobotWithLock(userid, robot)) {
        robot_map_[userid] = std::make_shared<Robot>(userid);
        return kCommSucc;
    }

    if (kCommSucc != GetRobotWithLock(userid, robot)) {
        assert(false);
        return kCommFaild;
    }
    return kCommSucc;
}

ThreadID RobotGameManager::GetRobotNotifyThreadID() {
    std::lock_guard<std::mutex> lock(robot_map_mutex_);
    return robot_notify_thread_.ThreadId();
}

// 具体业务

int RobotGameManager::SendGamePluse() {
    std::lock_guard<std::mutex> lock(robot_map_mutex_);
    for (auto& kv : robot_map_) {
        auto robot = kv.second;
        robot->SendGamePulse();
    }

    return kCommSucc;
}

int RobotGameManager::GetRobotWithLock(const UserID userid, RobotPtr& robot) const {
    CHECK_USERID(userid);
    auto& iter = robot_map_.find(userid);
    if (iter == robot_map_.end()) {
        assert(false);
        return kCommFaild;
    }
    robot = iter->second;
    return kCommSucc;
}

int RobotGameManager::SetRobotWithLock(RobotPtr robot) {
    CHECK_ROBOT(robot);
    robot_map_.insert(std::make_pair(robot->GetUserID(), robot));
    return kCommSucc;
}

int RobotGameManager::GetRobotByTokenWithLock(const TokenID token_id, RobotPtr& robot) const {
    CHECK_TOKENID(token_id);
    auto it = std::find_if(robot_map_.begin(), robot_map_.end(), [&] (const std::pair<UserID, RobotPtr>& it) {
        return it.second->GetTokenID() == token_id;
    });
    robot = (it != robot_map_.end() ? it->second : nullptr);
    return kCommSucc;
}


int RobotGameManager::ThreadRobotPluse() {
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
    return kCommSucc;
}

int RobotGameManager::ThreadRobotNotify() {
    UWL_INF(_T("Robot Notify thread started. id = %d"), GetCurrentThreadId());

    MSG msg = {};
    while (GetMessage(&msg, 0, 0, 0)) {
        if (UM_DATA_RECEIVED == msg.message) {

            LPCONTEXT_HEAD	pContext = (LPCONTEXT_HEAD) (msg.wParam);
            LPREQUEST		pRequest = (LPREQUEST) (msg.lParam);

            TokenID		nTokenID = pContext->lTokenID;
            RequestID		requestid = pRequest->head.nRequest;

            OnRobotNotify(requestid, pRequest->pDataPtr, pRequest->nDataLen, nTokenID);

            SAFE_DELETE(pContext);
            UwlClearRequest(pRequest);
            SAFE_DELETE(pRequest);
        } else {
            DispatchMessage(&msg);
        }
    }
    UWL_INF(_T("Robot Notify thread exiting. id = %d"), GetCurrentThreadId());
    return kCommSucc;
}

int RobotGameManager::OnRobotNotify(const RequestID requestid, void* ntf_data_ptr, const int data_size, const TokenID token_id) {
    RobotPtr robot;
    {
        std::lock_guard<std::mutex> lock(robot_map_mutex_);
        if (kCommSucc != GetRobotByTokenWithLock(token_id, robot))
            return kCommFaild;
    }

    switch (requestid) {
        case UR_SOCKET_ERROR:
        case UR_SOCKET_CLOSE:
            robot->DisconnectGame();
            break;
        default:
            break;
    }
    return kCommSucc;
}
