﻿#include "stdafx.h"
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
    LOG_FUNC("[START ROUTINE]");
    robot_heart_timer_thread_.Initial(std::thread([this] {this->ThreadRobotPluse(); }));
    robot_notify_thread_.Initial(std::thread([this] {this->ThreadRobotNotify(); }));
    UWL_INF("RobotGameManager::Init Sucessed.");
    return kCommSucc;
}

int RobotGameManager::Term() {
    robot_heart_timer_thread_.Release();
    robot_notify_thread_.Release();
    LOG_FUNC("[EXIT ROUTINE]");
    return kCommSucc;
}

int RobotGameManager::GetRobotWithCreate(const UserID userid, RobotPtr& robot) {
    CHECK_USERID(userid);
    std::lock_guard<std::mutex> lock(robot_map_mutex_);
    if (kCommSucc != GetRobotWithLock(userid, robot)) {
        robot = std::make_shared<Robot>(userid);
        SetRobotWithLock(robot);
    }

    if (kCommSucc != GetRobotWithLock(userid, robot)) {
        ASSERT_FALSE;
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
        if (robot->IsConnected()) {
            robot->SendGamePulse();
        }
    }

    return kCommSucc;
}

int RobotGameManager::GetRobotWithLock(const UserID userid, RobotPtr& robot) const {
    CHECK_USERID(userid);
    auto& iter = robot_map_.find(userid);
    if (iter == robot_map_.end()) {
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
    LOG_INFO("[START ROUTINE] RobotGameManager KeepAlive thread [%d] started", GetCurrentThreadId());
    while (true) {
        DWORD dwRet = WaitForSingleObject(g_hExitServer, PluseInterval);
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }

        if (WAIT_TIMEOUT == dwRet) {
            SendGamePluse();
        }
    }
    LOG_INFO("[EXIT ROUTINE] RobotGameManager KeepAlive thread [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}

int RobotGameManager::ThreadRobotNotify() {
    LOG_INFO("[START ROUTINE] RobotGameManager Notify thread [%d] started", GetCurrentThreadId());

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
    LOG_INFO("[EXIT ROUTINE] RobotGameManager Notify thread [%d] exiting", GetCurrentThreadId());
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

int RobotGameManager::SnapShotObjectStatus() {
    std::lock_guard<std::mutex> lock(robot_map_mutex_);
    LOG_FUNC("[SNAPSHOT] BEG");

    LOG_INFO("OBJECT ADDRESS [%x]", this);
    LOG_INFO("robot_notify_thread_ [%d]", robot_notify_thread_.ThreadId());
    LOG_INFO("robot_heart_timer_thread_ [%d]", robot_heart_timer_thread_.ThreadId());

    LOG_INFO("robot_map_ size [%d]", robot_map_.size());
    std::string str = "{";
    for (auto& kv : robot_map_) {
        auto userid = kv.first;
        auto robot = kv.second;
        auto token = robot->GetTokenID();
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

    LOG_INFO("%s", str.c_str());
    LOG_FUNC("[SNAPSHOT] END");
    return kCommSucc;
}


