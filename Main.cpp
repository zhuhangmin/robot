// Main.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "main.h"
#include "server.h"
#include "robot_define.h"
#pragma comment(lib,  "dbghelp.lib")
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;

int					g_nClientID = 0;

string			    g_curExePath;

TCHAR				g_szIniFile[MAX_PATH];//�����ļ�

HANDLE				g_hExitServer = NULL;

string	        	g_localGameIP;

CWinApp				theApp;

HINSTANCE			g_hResDll = NULL;

void WriteMiniDMP(struct _EXCEPTION_POINTERS *pExp) {
    CString   strDumpFile;
    TCHAR szFilePath[MAX_PATH];
    GetModuleFileName(NULL, szFilePath, MAX_PATH);
    *strrchr(szFilePath, '\\') = 0;
    strDumpFile.Format("%s\\%d.dmp", szFilePath, CTime::GetCurrentTime().GetTickCount());
    HANDLE   hFile = CreateFile(strDumpFile, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION   ExInfo;
        ExInfo.ThreadId = ::GetCurrentThreadId();
        ExInfo.ExceptionPointers = pExp;
        ExInfo.ClientPointers = NULL;
        //   write   the   dump 
        BOOL   bOK = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL);
        CloseHandle(hFile);
    }
}

LONG WINAPI ExpFilter(struct _EXCEPTION_POINTERS *pExp) {
    WriteMiniDMP(pExp);
    return EXCEPTION_EXECUTE_HANDLER;
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[]) {
    int nRetCode = 0;
    ::SetUnhandledExceptionFilter(ExpFilter);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    // ��ʼ�� MFC ����ʧ��ʱ��ʾ����
    if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0)) {
        MessageBox(NULL, _T("Fatal error: MFC initialization failed!\n"), "", MB_ICONSTOP);
        nRetCode = -1;
        return nRetCode;
    }

    if (!UwlInit()) {
        MessageBox(NULL, _T("Fatal error: UWL initialization failed!\n"), "", MB_ICONSTOP);
        nRetCode = -1;
        return nRetCode;
    }

    DWORD dwTraceMode = UWL_TRACE_DATETIME | UWL_TRACE_FILELINE | UWL_TRACE_NOTFULLPATH
        | UWL_TRACE_FORCERETURN | UWL_TRACE_CONSOLE;
    //| UWL_TRACE_FORCERETURN | UWL_TRACE_DUMPFILE | UWL_TRACE_CONSOLE;

    UwlBeginTrace((TCHAR*) AfxGetAppName(), dwTraceMode);
    UwlBeginLog((TCHAR*) AfxGetAppName());

    UwlRegSocketWnd();

    if (!AfxSocketInit()) {
        MessageBox(NULL, _T("Fatal error: Failed to initialize sockets!\n"), "", MB_ICONSTOP);
        nRetCode = -1;
        return nRetCode;
    }


#ifdef UWL_SERVICE

    TCHAR szIniFile[MAX_PATH] = {0};
    TCHAR szExeName[MAX_PATH] = {0};
    lstrcpy(szExeName, argv[0]);
    PathStripPath(szExeName);
    PathRemoveExtension(szExeName);
    TCHAR szExeIni[MAX_PATH] = {0};
    sprintf(szExeIni, "%s.ini", szExeName);

    TCHAR szFullName[MAX_PATH] = {0};
    GetModuleFileName(GetModuleHandle(NULL), szFullName, sizeof(szFullName));
    UwlSplitPath(szFullName, SPLIT_DRIVE_DIR, szIniFile);
    lstrcat(szIniFile, szExeIni);

    TCHAR serviceName[MAX_PATH] = {0};
    GetPrivateProfileString("service", "service_name", SERVICE_NAME, serviceName, MAX_PATH, szIniFile);

    TCHAR displayName[MAX_PATH] = {0};
    GetPrivateProfileString("service", "display_name", DISPLAY_NAME, displayName, MAX_PATH, szIniFile);

    if (strcmp(szExeName, serviceName) != 0) {
        UWL_ERR("szExeName[%s] != serviceName[%s]", szExeName, serviceName);
        assert(false);
        nRetCode = -1;
        return nRetCode;
    }
    CMainService MainService(serviceName, displayName, 0, 0);

    if (!MainService.ParseStandardArgs(argc, argv)) {
        // Didn't find any standard args so start the service
        // Uncomment the DebugBreak line below to enter the debugger when the service is started.
        //DebugBreak();
        MainService.StartService();
    }
    // When we get here, the service has been stopped
    nRetCode = MainService.m_Status.dwWin32ExitCode;
#else

    MainServer MainServer;

    if (kCommFaild == MainServer.Init()) {
        UwlTrace(_T("server initialize failed!"));
        MainServer.Term();
        return -1;
    }

    UwlTrace("Type 'q' when you want to exit. ");
    TCHAR ch;
    do {
        ch = _getch();
        ch = toupper(ch);

    } while (ch != 'Q');

    MainServer.Term();

    nRetCode = 1;
#endif

    if (g_hResDll) {
        AfxFreeLibrary(g_hResDll);
    }
    UwlEndLog();
    UwlEndTrace();
    UwlTerm();
    WSACleanup();
    return nRetCode;
}
