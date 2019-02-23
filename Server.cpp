#include "stdafx.h"
#include "server.h"
#include "main.h"
#include "robot_game_manager.h"
#include "setting_manager.h"
#include "game_info_manager.h"
#include "robot_deposit_manager.h"
#include "robot_hall_manager.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define	MAX_PROCESSORS_SUPPORT	4

#define LICENSE_FILE		_T("license.dat")
#define PRODUCT_NAME		_T("RobotToolLudo")
#define	PRODUCT_VERSION		_T("1.00")


#define MAX_FORBIDDEN_IPS		100 // 

#define	DEF_KICKOFF_DEADTIME	360			// minutes
#define	DEF_KICKOFF_STIFFTIME	300			// seconds

CMainServer::CMainServer() {
    g_hExitServer = NULL;
}

CMainServer::~CMainServer() {}

int CMainServer::InitLanuch() {
    g_hExitServer = CreateEvent(NULL, TRUE, FALSE, NULL);

    TCHAR szFullName[MAX_PATH];
    GetModuleFileName(GetModuleHandle(NULL), szFullName, sizeof(szFullName));
    UwlSplitPath(szFullName, SPLIT_DRIVE_DIR, g_szIniFile);
    g_curExePath = g_szIniFile;
    lstrcat(g_szIniFile, PRODUCT_NAME);
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

int CMainServer::Init() {
    UwlLogFile(_T("server starting..."));

    if (S_FALSE == ::CoInitialize(NULL))
        return kCommFaild;;

    if (kCommFaild == InitLanuch()) {
        UWL_ERR(_T("InitBase() return failed"));
        assert(false);
        return kCommFaild;
    }

    // ����������
    if (kCommFaild == SettingMgr.Init()) {
        UWL_ERR(_T("SettingManager Init Failed"));
        assert(false);
        return kCommFaild;
    }

    // ��Ϸ�������ݹ����� TODO
    /*if (kCommFaild == GameMgr.Init()) {
        UWL_ERR(_T("RobotGameInfoManager Init Failed"));
        assert(false);
        return kCommFaild;
        }*/

    // �����˴���������
    if (kCommFaild == HallMgr.Init()) {
        UWL_ERR(_T("RobotHallManager Init Failed"));
        assert(false);
        return kCommFaild;
    }

    // ��������Ϸ������
    if (kCommFaild == RobotMgr.Init()) {
        UWL_ERR(_T("RobotGameManager Init Failed"));
        assert(false);
        return kCommFaild;
    }

    // �����˲���������
    if (kCommFaild == DepositMgr.Init()) {
        UWL_ERR(_T("RobotDepositManager Init Failed"));
        assert(false);
        return kCommFaild;
    }

    // ������
    main_timer_thread_.Initial(std::thread([this] {this->ThreadMainProc(); }));

    UwlTrace(_T("Server start up OK."));
    UwlLogFile(_T("Server start up OK."));
    return kCommSucc;
}

void CMainServer::Term() {
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

void CMainServer::ThreadMainProc() {
    UwlTrace(_T("timer thread started. id = %d"), GetCurrentThreadId());
    while (true) {
        DWORD dwRet = WaitForSingleObject(g_hExitServer, SettingMgr.GetMainsInterval());
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }
        if (WAIT_TIMEOUT == dwRet) { // timeout
            //UWL_DBG(_T("[interval] ---------------------- timer thread triggered. do something. interval = %ld ms."), DEF_TIMER_INTERVAL);
            //UWL_DBG("[interval] TimerThreadProc = %I32u", time(nullptr));

            //TODO ���ѡһ��û�н�����Ϸ��robot
            //FixMe��
            UserID random_userid = InvalidUserID;
            if (kCommFaild == HallMgr.GetRandomNotLogonUserID(random_userid))
                continue;

            if (InvalidUserID == random_userid)
                continue;

            if (kCommFaild == HallMgr.LogonHall(random_userid))
                continue;


            RobotPtr robot = RobotMgr.GetRobotWithCreate(random_userid);

            //TODO designed roomid
            RoomID designed_roomid = 7846; //FixMe: hard code
            HallRoomData hall_room_data;
            if (kCommFaild == HallMgr.GetHallRoomData(designed_roomid, hall_room_data))
                continue;

            //@zhuhangmin 20190223 issue: ����ⲻ֧������IPV6������ʹ������IP
            auto game_ip = SettingMgr.GetGameIP();
            auto game_port = hall_room_data.room.nPort;
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
