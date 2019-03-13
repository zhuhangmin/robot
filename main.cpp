#include "stdafx.h"
#include "main.h"
#include "app_delegate.h"
#include "robot_define.h"
#include "setting_manager.h"
#include "robot_net_manager.h"
#include "game_net_manager.h"
#include "robot_deposit_manager.h"
#include "robot_hall_manager.h"
#include "user_manager.h"
#include "room_manager.h"
#include "process_info.h"
#include "robot_utils.h"
#include "robot_statistic.h"
#pragma comment(lib,  "dbghelp.lib")

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new DEBUG_NEW
#endif

using namespace std;

int					g_nClientID = 0;

string			    g_curExePath;

TCHAR				g_szIniFile[MAX_PATH];//配置文件

HANDLE				g_hExitServer = NULL;

string	        	g_localGameIP;

CWinApp				theApp;

HINSTANCE			g_hResDll = NULL;

extern void Test(char cmd, AppDelegate& app_delegate);

void WriteMiniDMP(struct _EXCEPTION_POINTERS *pExp) {
    CString   strDumpFile;
    TCHAR szFilePath[MAX_PATH];
    GetModuleFileName(NULL, szFilePath, MAX_PATH);
    *strrchr(szFilePath, '\\') = 0;
    SYSTEMTIME stTime;
    GetLocalTime(&stTime);
    strDumpFile.Format("%s\\[%02d-%02d %02d-%02d-%02d]full.dmp", szFilePath, stTime.wMonth, stTime.wDay, stTime.wHour, stTime.wMinute, stTime.wSecond);
    HANDLE   hFile = CreateFile(strDumpFile, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION   ExInfo;
        ExInfo.ThreadId = ::GetCurrentThreadId();
        ExInfo.ExceptionPointers = pExp;
        ExInfo.ClientPointers = NULL;
        (void) MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, (MINIDUMP_TYPE) 0x9b67, &ExInfo, NULL, NULL);
        (void) CloseHandle(hFile);
    }
}

LONG WINAPI ExpFilter(struct _EXCEPTION_POINTERS *pExp) {
    WriteMiniDMP(pExp);
    return EXCEPTION_EXECUTE_HANDLER;
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[]) {
    TCLOG_INIT();
    LOG_INFO("\n ===================================SERVER START===================================");
    LOG_INFO("*********[START BEG]*********");

    //绑定单核运行 避免机器人本身消耗所有CPU
    BOOL success = SetProcessAffinityMask(GetCurrentProcess(), 0x00000001);
    if (!success) {
        LOG_WARN("bind to single core 0x00000001 failed error [%d]", GetLastError());
    }

    auto nRetCode = EXIT_SUCCESS;
    SetUnhandledExceptionFilter(ExpFilter);

    LOG_INFO("\t[START] AFX BEG");
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    // 初始化 MFC 并在失败时显示错误
    if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0)) {
        MessageBox(NULL, _T("Fatal error: MFC initialization failed!\n"), "", MB_ICONSTOP);
        nRetCode = EXIT_FAILURE;
        return nRetCode;
    }
    LOG_INFO("\t[START] AFX END");


    LOG_INFO("\t[START] UWL BEG");
    if (!UwlInit()) {
        MessageBox(NULL, _T("Fatal error: UWL initialization failed!\n"), "", MB_ICONSTOP);
        nRetCode = EXIT_FAILURE;
        return nRetCode;
    }
    LOG_INFO("\t[START] UWL END");

    DWORD dwTraceMode = UWL_TRACE_DATETIME | UWL_TRACE_FILELINE | UWL_TRACE_NOTFULLPATH
        | UWL_TRACE_FORCERETURN | UWL_TRACE_CONSOLE;
    //| UWL_TRACE_FORCERETURN | UWL_TRACE_DUMPFILE | UWL_TRACE_CONSOLE;

    UwlBeginTrace((TCHAR*) AfxGetAppName(), dwTraceMode);
    UwlBeginLog((TCHAR*) AfxGetAppName());

    UwlRegSocketWnd();

    LOG_INFO("\t[START] AFX SOCKET BEG");
    if (!AfxSocketInit()) {
        MessageBox(NULL, _T("Fatal error: Failed to initialize sockets!\n"), "", MB_ICONSTOP);
        nRetCode = EXIT_FAILURE;
        return nRetCode;
    }
    LOG_INFO("\t[START] AFX SOCKET END");

#ifdef UWL_SERVICE

    LOG_INFO("\t[START] SERVICE BEG");
    TCHAR szIniFile[MAX_PATH] = {0};
    TCHAR szExeName[MAX_PATH] = {0};
    lstrcpy(szExeName, argv[0]);
    PathStripPath(szExeName);
    PathRemoveExtension(szExeName);
    TCHAR szExeIni[MAX_PATH] = {0};
    sprintf(szExeIni, " [%s].ini", szExeName);

    TCHAR szFullName[MAX_PATH] = {0};
    GetModuleFileName(GetModuleHandle(NULL), szFullName, sizeof(szFullName));
    UwlSplitPath(szFullName, SPLIT_DRIVE_DIR, szIniFile);
    lstrcat(szIniFile, szExeIni);

    TCHAR serviceName[MAX_PATH] = {0};
    GetPrivateProfileString("service", "service_name", SERVICE_NAME, serviceName, MAX_PATH, szIniFile);

    TCHAR displayName[MAX_PATH] = {0};
    GetPrivateProfileString("service", "display_name", DISPLAY_NAME, displayName, MAX_PATH, szIniFile);

    if (strcmp(szExeName, serviceName) != 0) {
        LOG_ERROR("szExeName[ [%s]] != serviceName[ [%s]]", szExeName, serviceName);
        nRetCode = EXIT_FAILURE;
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
    LOG_INFO("\t[START] AppDelegate  BEG");
    AppDelegate app_delegate;

    if (kCommSucc != app_delegate.Init()) {
        LOG_ERROR("server initialize failed!");
        UwlTrace("server initialize failed!");
        app_delegate.Term();
        return -1;
    }

    LOG_INFO("\t[START] AppDelegate END");
    LOG_INFO("*********[START END]*********");

    UwlTrace("Type 'q' when you want to exit. ");
    TCHAR ch;
    do {
        ch = _getch();
        ch = toupper(ch);
#ifdef _DEBUG
        Test(ch, app_delegate);
#endif

    } while (ch != 'Q');

    app_delegate.Term();

    nRetCode = EXIT_SUCCESS;
#endif

    TCLOG_UNINT();
    if (g_hResDll) {
        AfxFreeLibrary(g_hResDll);
    }
    UwlEndLog();
    UwlEndTrace();
    UwlTerm();
    WSACleanup();

    _CrtDumpMemoryLeaks();
    return nRetCode;
}

// TEST CASE
extern void Test(char cmd, AppDelegate& app_delegate) {
    if (cmd == 'S') {
        LOG_INFO("-------------[STATUS SNAPSHOT BEG]-------------");
        SettingMgr.SnapShotObjectStatus();
        RobotMgr.SnapShotObjectStatus();
        GameMgr.SnapShotObjectStatus();
        DepositMgr.SnapShotObjectStatus();
        HallMgr.SnapShotObjectStatus();
        UserMgr.SnapShotObjectStatus();
        RoomMgr.SnapShotObjectStatus();
        LOG_INFO("-------------[STATUS SNAPSHOT END]-------------");
    }

    if (cmd == 'B') {
        LOG_INFO("-------------[STATUS SNAPSHOT BEG]-------------");
        SettingMgr.BriefInfo();
        RobotMgr.BriefInfo();
        HallMgr.BriefInfo();
        UserMgr.BriefInfo();
        LOG_INFO("-------------[STATUS SNAPSHOT END]-------------");
    }

    if (cmd == 'D') {
        LOG_INFO("-------------[DEPOSIT TEST BEG]-------------");
        auto robot_setting_map = SettingMgr.GetRobotSettingMap();
        for (auto& kv : robot_setting_map) {
            auto userid = kv.first;
            DepositMgr.SetDepositType(userid, DepositType::kGain);
        }

        LOG_INFO("-------------[DEPOSIT TEST END]-------------");
    }

    if (cmd == 'P') {
        LOG_INFO("-------------[PROCESS TEST BEG]-------------");
        ProcessInfoCollect picProcessInfoCollect;
        int nRet = 0;
        DWORD             nMemoryUsed;                    //内存使用(Byte)    
        DWORD            nVirtualMemoryUsed;                //虚拟内存使用(Byte)    
        DWORD            nHandleNumber;                    //句柄数量
        DWORD dwCurrentProcessThreadCount;        //线程数量    
        ULONGLONG ullIo_read_bytes;                        //IO读字节数    
        ULONGLONG ullIo_write_bytes;                    //IO写字节数    
        ULONGLONG ullIo_wct;                            //IO写次数    
        ULONGLONG ullIo_rct;                            //IO读次数        
        double dCPUUserRate = 0;                        //CPU使用的百分比        
        picProcessInfoCollect.GetCPUUserRate(dCPUUserRate);
        picProcessInfoCollect.GetMemoryUsed(nVirtualMemoryUsed, nMemoryUsed);
        nVirtualMemoryUsed = nVirtualMemoryUsed;
        nMemoryUsed = nMemoryUsed;
        picProcessInfoCollect.GetThreadCount(dwCurrentProcessThreadCount);
        picProcessInfoCollect.GetHandleCount(nHandleNumber);
        picProcessInfoCollect.GetIOBytes(&ullIo_read_bytes, &ullIo_write_bytes, &ullIo_wct, &ullIo_rct);
        LOG_INFO("cpu [%d], memory [%d] MB, handler count [%d], thread count [%d]", (int) dCPUUserRate, (int) nMemoryUsed / 1024 /1024, (int) nHandleNumber, (int) dwCurrentProcessThreadCount);
        LOG_INFO("-------------[PROCESS TEST END]-------------");
    }

    if (cmd == 'T') {
        LOG_INFO("-------------[CONNECTION COST TEST BEG]-------------");
        auto test_count = 100;
        for (auto i = 0; i < test_count; i++) {
            TCHAR szHallSvrIP[MAX_SERVERIP_LEN] = {};
            GetPrivateProfileString(_T("hall_server"), _T("ip"), _T(""), szHallSvrIP, sizeof szHallSvrIP, g_szIniFile);
            auto nHallSvrPort = GetPrivateProfileInt(_T("hall_server"), _T("port"), 0, g_szIniFile);
            CDefSocketClientPtr connection = std::make_shared<CDefSocketClient>();
            connection->InitKey(KEY_HALL, ENCRYPT_AES, 0);
            connection->Create(szHallSvrIP, nHallSvrPort, 5, 0, GetCurrentThreadId(), 0, GetHelloData(), GetHelloLength());
        }
        LOG_INFO("-------------[CONNECTION COST TEST END]-------------");
    }

    if (cmd == 'M') {
        LOG_INFO("-------------[MSG COUNT TEST BEG]-------------");
        RobotStatistic::Instance().SnapShotObjectStatus();
        LOG_INFO("-------------[MSG COUNT TEST END]-------------");
    }


    if (cmd == 'A') {
        LOG_INFO("-------------[ADD ROBOT TEST BEG]-------------");
        auto userid = 685555;
        auto roomid = 7972;
        auto tableno = 1;
        app_delegate.TestLogonHall(userid);
        app_delegate.SendTestRobot(userid, roomid, tableno);
        LOG_INFO("-------------[ADD ROBOT TEST END]-------------");
    }

}