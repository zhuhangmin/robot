#include "stdafx.h"
#include "server.h"
#include "main.h"
#include "robot_manager.h"
#include "setting_manager.h"
#include "game_info_manager.h"
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

    if (kCommFaild == SettingManager::Instance().Init()) {
        UWL_ERR(_T("SettingManager Init Failed"));
        assert(false);
        return kCommFaild;
    }

    /*if (kCommFaild == GameInfoManager::Instance().Init()) {
        UWL_ERR(_T("GameInfoManager Init Failed"));
        assert(false);
        return kCommFaild;
        }*/

    if (kCommFaild == CRobotMgr::Instance().Init()) {
        UWL_ERR(_T("CRobotMgr Init Failed"));
        assert(false);
        return kCommFaild;
    }

    ////////////////////////////////////////////////////////
    UwlTrace(_T("Server start up OK."));
    UwlLogFile(_T("Server start up OK."));
    return kCommSucc;
}

void CMainServer::Term() {
    SetEvent(g_hExitServer);

    CRobotMgr::Instance().Term();
    GameInfoManager::Instance().Term();
    //TODO TERM ALL THE MANAGER
    if (g_hExitServer) { CloseHandle(g_hExitServer); g_hExitServer = NULL; }

    ::CoUninitialize();
    UwlLogFile(_T("server exited."));
}

