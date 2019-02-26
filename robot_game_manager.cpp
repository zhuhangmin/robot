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

void RobotGameManager::Term() {
    robot_heart_timer_thread_.Release();
    robot_notify_thread_.Release();
}

int RobotGameManager::GetRobotWithCreate(UserID userid, RobotPtr& robot) {
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

void RobotGameManager::SendGamePluse() {
    std::lock_guard<std::mutex> lock(robot_map_mutex_);
    for (auto& kv : robot_map_) {
        auto robot = kv.second;
        robot->SendGamePulse();
    }
}

int RobotGameManager::GetRobotWithLock(UserID userid, RobotPtr& robot) {
    CHECK_USERID(userid);
    CHECK_ROBOT(robot);
    if (robot_map_.find(userid) == robot_map_.end()) {
        assert(false);
        return kCommFaild;
    }
    robot = robot_map_[userid];
    return kCommSucc;
}

int RobotGameManager::SetRobotWithLock(RobotPtr robot) {
    CHECK_ROBOT(robot);
    robot_map_.insert(std::make_pair(robot->GetUserID(), robot));
    return kCommSucc;
}

int RobotGameManager::GetRobotByTokenWithLock(const TokenID token_id, RobotPtr& robot) {
    CHECK_TOKENID(token_id);
    auto it = std::find_if(robot_map_.begin(), robot_map_.end(), [&] (const std::pair<UserID, RobotPtr>& it) {
        return it.second->GetTokenID() == token_id;
    });
    robot = (it != robot_map_.end() ? it->second : nullptr);
    return kCommSucc;
}


void RobotGameManager::ThreadRobotPluse() {
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

void RobotGameManager::ThreadRobotNotify() {
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
    return;
}

int RobotGameManager::OnRobotNotify(RequestID requestid, void* ntf_data_ptr, int data_size, TokenID token_id) {
    CHECK_REQUESTID(requestid);
    RobotPtr robot;
    {
        std::lock_guard<std::mutex> lock(robot_map_mutex_);
        auto result = GetRobotByTokenWithLock(token_id, robot);
        if (kCommSucc != result)
            return result;
    }

    if (!robot) {
        assert(false);
        UWL_WRN(_T("GameNotify robot not found. nTokenID = %d reqId:%d"), token_id, requestid);
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
