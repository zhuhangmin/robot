#include "stdafx.h"
#include "Server.h"
#include "Main.h"
#include "RobotMgr.h"
#include "setting_manager.h"
#include "GameInfoManager.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define	MAX_PROCESSORS_SUPPORT	4

#define LICENSE_FILE		_T("license.dat")
#define PRODUCT_NAME		_T("RobotToolLudo")
#define	PRODUCT_VERSION		_T("1.00")

//#define DEF_REFRESH_ELAPSE		(DEF_TIMER_INTERVAL * 12) // minitues

#define MAX_FORBIDDEN_IPS		100 // 

#define	DEF_KICKOFF_DEADTIME	360			// minutes
#define	DEF_KICKOFF_STIFFTIME	300			// seconds

CMainServer::CMainServer() {
    g_hExitServer = NULL;
}

CMainServer::~CMainServer() {}

BOOL CMainServer::InitBase() {
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
        UwlLogFile(_T("invalid client id!"));
        return FALSE;
    } else {
        UwlLogFile(_T("client id=%d!"), g_nClientID);
    }
    g_useLocal = GetPrivateProfileInt(_T("LocalNet"), _T("Enable"), 0, g_szIniFile);
    UwlTrace(_T("local ip enable=%d!"), g_useLocal);
    UwlLogFile(_T("local ip enable=%d!"), g_useLocal);
    return TRUE;
}

BOOL CMainServer::Initialize() {
    UwlLogFile(_T("server starting..."));

    if (S_FALSE == ::CoInitialize(NULL))
        return FALSE;

    if (!InitBase()) {
        UWL_ERR(_T("InitBase() return false"));
        assert(false);
        return FALSE;
    }

    if (!SettingManager::Instance().Init()) {
        UWL_ERR(_T("SettingManager Init Failed"));
        assert(false);
        return FALSE;
    }

    if (!GameInfoManager::Instance().Init()) {
        UWL_ERR(_T("GameInfoManager Init Failed"));
        assert(false);
        return FALSE;
    }

    if (!TheRobotMgr.Init()) {
        UWL_ERR(_T("TheRobotMgr Init Failed"));
        assert(false);
        return FALSE;
    }



    ////////////////////////////////////////////////////////
    UwlTrace(_T("Server start up OK."));
    UwlLogFile(_T("Server start up OK."));
    return TRUE;
}

void CMainServer::Shutdown() {
    SetEvent(g_hExitServer);



    TheRobotMgr.Term();
    GameInfoManager::Instance().Term();


    if (g_hExitServer) { CloseHandle(g_hExitServer); g_hExitServer = NULL; }

    ::CoUninitialize();
    UwlLogFile(_T("server exited."));
}

