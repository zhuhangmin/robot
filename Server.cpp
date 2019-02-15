#include "stdafx.h"
#include "Server.h"
#include <json.h>
#include "Main.h"
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

    /*if (!TheRobotMgr.Init()) {
        UWL_ERR(_T("TheRobotMgr Init() return false"));
        assert(false);
        return FALSE;
        }*/

    g_thrdTimer.Initial(std::thread([this] {this->TimerThreadProc(); }));

    ////////////////////////////////////////////////////////
    StartServer();
    return TRUE;
}
BOOL CMainServer::StartServer() {
    UwlTrace(_T("Server start up OK."));
    UwlLogFile(_T("Server start up OK."));

    int nRobotPort = GetPrivateProfileInt(_T("listen"), _T("port"), PORT_ROBOT_SERVER, g_szIniFile);

    SYSTEM_INFO SystemInfo;
    ZeroMemory(&SystemInfo, sizeof(SystemInfo));
    GetSystemInfo(&SystemInfo);
    UwlTrace(_T("number of processors: %lu"), SystemInfo.dwNumberOfProcessors);
    UwlLogFile(_T("number of processors: %lu"), SystemInfo.dwNumberOfProcessors);

    return true;
}
void CMainServer::Shutdown() {
    SetEvent(g_hExitServer);

    g_thrdTimer.Release();

    //TheRobotMgr.Term();


    if (g_hExitServer) { CloseHandle(g_hExitServer); g_hExitServer = NULL; }

    ::CoUninitialize();
    UwlLogFile(_T("server exited."));
}

void CMainServer::TimerThreadProc() {
    UwlTrace(_T("timer thread started. id = %d"), GetCurrentThreadId());

    while (TRUE) {
        DWORD dwRet = WaitForSingleObject(g_hExitServer, DEF_TIMER_INTERVAL);
        if (WAIT_OBJECT_0 == dwRet) { // exit event
            break;
        }
        if (WAIT_TIMEOUT == dwRet) { // timeout
            //UWL_DBG(_T("[interval] ---------------------- timer thread triggered. do something. interval = %ld ms."), DEF_TIMER_INTERVAL);
            //UWL_DBG("[interval] TimerThreadProc = %I32u", time(nullptr));
            OnThreadTimer(time(nullptr));
        }
    }

    UwlLogFile(_T("timer thread exiting. id = %d"), GetCurrentThreadId());
    return;
}
void CMainServer::OnThreadTimer(time_t nCurrTime) {
    //UWL_DBG("[interval] OnThreadTimer = %I32u", time(nullptr));
    //TheRobotMgr.OnServerMainTimer(nCurrTime);

#define MAIN_XXX_GAP_TIME (10*60) // 10·ÖÖÓ
    static time_t	sLastXXXTime = 0;
    if (nCurrTime - sLastXXXTime >= MAIN_XXX_GAP_TIME) {
        sLastXXXTime = nCurrTime;
    }

    // ... like up
}