#include "stdafx.h"
#include "main_server.h"
#include "main.h"
#include "robot_game_manager.h"
#include "setting_manager.h"
#include "game_info_manager.h"
#include "robot_deposit_manager.h"
#include "robot_hall_manager.h"
#include "robot_utils.h"
#include "user_manager.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

MainServer::MainServer() {
    g_hExitServer = NULL;
}

MainServer::~MainServer() {}

int MainServer::InitLanuch() {
    LOG_FUNC("[START ROUTINE]");
    g_hExitServer = CreateEvent(NULL, TRUE, FALSE, NULL);

    TCHAR szFullName[MAX_PATH];
    GetModuleFileName(GetModuleHandle(NULL), szFullName, sizeof(szFullName));
    UwlSplitPath(szFullName, SPLIT_DRIVE_DIR, g_szIniFile);
    g_curExePath = g_szIniFile;
    lstrcat(g_szIniFile, "RobotTool");
    lstrcat(g_szIniFile, _T(".ini"));

    TCHAR szPID[32];
    sprintf_s(szPID, _T("%d"), GetCurrentProcessId());
    WritePrivateProfileString(_T("listen"), _T("pid"), szPID, g_szIniFile);

    g_nClientID = GetPrivateProfileInt(_T("listen"), _T("clientid"), 0, g_szIniFile);
    if (0 == g_nClientID) {
        LOG_ERROR("[START ROUTINE] invalid clientid 0!");
        assert(false);
        return kCommFaild;

    } else {
        LOG_INFO("[START ROUTINE] clientid = [%d]", g_nClientID);
    }
    return kCommSucc;
}

int MainServer::Init() {
    LOG_FUNC("[START ROUTINE]");
    if (S_FALSE == ::CoInitialize(NULL))
        return kCommFaild;;

    if (kCommFaild == InitLanuch()) {
        UWL_ERR(_T("InitBase() return failed"));
        assert(false);
        return kCommFaild;
    }

    // 配置数据类
    if (kCommFaild == SettingMgr.Init()) {
        UWL_ERR(_T("SettingManager Init Failed"));
        assert(false);
        return kCommFaild;
    }

    // 机器人大厅管理类
    if (kCommFaild == HallMgr.Init()) {
        UWL_ERR(_T("RobotHallManager Init Failed"));
        assert(false);
        return kCommFaild;
    }

    // 游戏服务数据管理类
    /*auto game_port = RobotUtils::GetGamePort();
    auto game_ip = RobotUtils::GetGameIP();
    if (kCommFaild == GameMgr.Init(game_ip, game_port)) {
    UWL_ERR(_T("RobotGameInfoManager Init Failed"));
    assert(false);
    return kCommFaild;
    }*/

    // 机器人游戏管理类
    if (kCommFaild == RobotMgr.Init()) {
        UWL_ERR(_T("RobotGameManager Init Failed"));
        assert(false);
        return kCommFaild;
    }

    // 机器人补银管理类
    if (kCommFaild == DepositMgr.Init()) {
        UWL_ERR(_T("RobotDepositManager Init Failed"));
        assert(false);
        return kCommFaild;
    }

    // 主流程
    main_timer_thread_.Initial(std::thread([this] {this->ThreadMainProc(); }));

    return kCommSucc;
}

int MainServer::Term() {
    SetEvent(g_hExitServer);

    DepositMgr.Term();
    RobotMgr.Term();
    HallMgr.Term();
    GameMgr.Term();
    SettingMgr.Term();

    main_timer_thread_.Release();

    if (g_hExitServer) { CloseHandle(g_hExitServer); g_hExitServer = NULL; }

    ::CoUninitialize();
    LOG_FUNC("[EXIT ROUTINE]");
    LOG_INFO("\n ===================================SERVER EXIT=================================== \n");
    return kCommSucc;

}

int MainServer::ThreadMainProc() {
    LOG_INFO("[START ROUTINE] main timer thread [%d] started", GetCurrentThreadId());
    while (true) {
        DWORD dwRet = WaitForSingleObject(g_hExitServer, SettingMgr.GetMainsInterval());
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }
        if (WAIT_TIMEOUT == dwRet) { // timeout
            //UWL_DBG(_T("[interval] ---------------------- timer thread triggered. do something. interval = %ld ms."), DEF_TIMER_INTERVAL);
            //UWL_DBG("[interval] TimerThreadProc = %I32u", time(nullptr));

            //TODO 随机选一个没有进入游戏的robot
            //FixMe：
            auto random_userid = InvalidUserID;
            if (kCommSucc != FindRandomUserIDNotInGame(random_userid)) {
                continue;
            }

            if (random_userid == InvalidUserID)
                continue;

            if (kCommSucc != HallMgr.LogonHall(random_userid))
                continue;



            //TODO designed roomid
            RoomID designed_roomid = 7846; //FixMe: hard code
            HallRoomData hall_room_data;
            if (kCommFaild == HallMgr.GetHallRoomData(designed_roomid, hall_room_data))
                continue;

            //@zhuhangmin 20190223 issue: 网络库不支持域名IPV6解析，使用配置IP
            auto game_ip = RobotUtils::GetGameIP();
            auto game_port = RobotUtils::GetGamePort();
            auto game_notify_thread_id = RobotMgr.GetRobotNotifyThreadID();

            RobotPtr robot;
            if (kCommSucc != RobotMgr.GetRobotWithCreate(random_userid, robot)) {
                assert(false);
                continue;
            }

            if (!robot) {
                assert(false);
                continue;
            }

            if (kCommFaild == robot->ConnectGame(game_ip, game_port, game_notify_thread_id))
                continue;

            if (kCommFaild == robot->SendEnterGame(designed_roomid))
                continue;

            //TODO 
            // HANDLE EXCEPTION
            // DEPOSIT OVERFLOW UNDERFLOW

        }
    }

    LOG_INFO("[EXIT ROUTINE] main timer thread [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}

int MainServer::FindRandomUserIDNotInGame(UserID& random_userid) {
    // 过滤出没有登入游戏服务器的userid集合
    std::hash_map<UserID, UserID> not_logon_game_temp;
    auto enter_game_map = UserMgr.GetAllEnterUserID();
    auto robot_setting = SettingMgr.GetRobotSettingMap();
    for (auto& kv : robot_setting) {
        auto userid = kv.first;
        if (enter_game_map.find(userid) == enter_game_map.end()) {
            not_logon_game_temp[userid] = userid;
        }
    }

    if (not_logon_game_temp.size() == 0) {
        LOG_WARN("no more robot !");
        assert(false);
        return kCommFaild;
    }

    // 随机选取
    auto random_pos = 0;
    if (kCommFaild == RobotUtils::GenRandInRange(0, not_logon_game_temp.size() - 1, random_pos)) {
        assert(false);
        return kCommFaild;
    }
    auto random_it = std::next(std::begin(not_logon_game_temp), random_pos);
    random_userid = random_it->first;

    return kCommSucc;
}
