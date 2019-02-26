#include "stdafx.h"
#include "server.h"
#include "main.h"
#include "robot_game_manager.h"
#include "setting_manager.h"
#include "game_info_manager.h"
#include "robot_deposit_manager.h"
#include "robot_hall_manager.h"
#include "robot_utils.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

MainServer::MainServer() {
    g_hExitServer = NULL;
}

MainServer::~MainServer() {}

int MainServer::InitLanuch() {
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
        UwlLogFile(_T("invalid clientid 0!"));
        return kCommFaild;

    } else {
        UwlLogFile(_T("clientid = %d!"), g_nClientID);
    }
    return kCommSucc;
}

int MainServer::Init() {
    UwlLogFile(_T("server starting..."));

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
    auto game_port = RobotUtils::GetGamePort();
    auto game_ip = RobotUtils::GetGameIP();
    if (kCommFaild == GameMgr.Init(game_ip, game_port)) {
        UWL_ERR(_T("RobotGameInfoManager Init Failed"));
        assert(false);
        return kCommFaild;
    }

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

    UwlTrace(_T("Server start up OK."));
    UwlLogFile(_T("Server start up OK."));
    return kCommSucc;
}

void MainServer::Term() {
    SetEvent(g_hExitServer);

    DepositMgr.Term();
    RobotMgr.Term();
    HallMgr.Term();
    GameMgr.Term();
    SettingMgr.Term();

    main_timer_thread_.Release();

    if (g_hExitServer) { CloseHandle(g_hExitServer); g_hExitServer = NULL; }

    ::CoUninitialize();
    UwlLogFile(_T("server exited."));
}

void MainServer::ThreadMainProc() {
    UwlTrace(_T("timer thread started. id = %d"), GetCurrentThreadId());
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
            UserID random_userid = InvalidUserID;
            if (kCommFaild == HallMgr.GetRandomNotLogonUserID(random_userid))
                continue;

            if (InvalidUserID == random_userid)
                continue;

            if (kCommFaild == HallMgr.LogonHall(random_userid))
                continue;

            if (random_userid <= InvalidUserID)
                continue;

            RobotPtr robot;
            if (kCommSucc != RobotMgr.GetRobotWithCreate(random_userid, robot)) {
                assert(false);
                continue;
            }

            //TODO designed roomid
            RoomID designed_roomid = 7846; //FixMe: hard code
            HallRoomData hall_room_data;
            if (kCommFaild == HallMgr.GetHallRoomData(designed_roomid, hall_room_data))
                continue;

            //@zhuhangmin 20190223 issue: 网络库不支持域名IPV6解析，使用配置IP
            auto game_ip = RobotUtils::GetGameIP();
            auto game_port = RobotUtils::GetGamePort();
            auto game_notify_thread_id = RobotMgr.GetRobotNotifyThreadID();
            if (kCommFaild == robot->ConnectGame(game_ip, game_port, game_notify_thread_id))
                continue;

            if (kCommFaild == robot->SendEnterGame(designed_roomid))
                continue;

            //TODO 
            // HANDLE EXCEPTION
            // DEPOSIT OVERFLOW UNDERFLOW

        }
    }

    UwlLogFile(_T("timer thread exiting. id = %d"), GetCurrentThreadId());
    return;
}
