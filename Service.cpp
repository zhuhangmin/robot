#include "stdafx.h"
#include "Service.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const DWORD dwWaitFinished = 5000; // time to wait for threads to finish up

CMainService::CMainService(const TCHAR* szServiceName, const TCHAR* szDisplayName,
                           const int iMajorVersion, const int iMinorVersion)
                           :CNTService(szServiceName, szDisplayName, iMajorVersion, iMinorVersion) {
    m_iStartParam = 0;
    m_iIncParam = 1;
    m_iState = m_iStartParam;
}

BOOL CMainService::OnInit() {
    // Read the registry parameters
    // Try opening the registry key:
    // HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\<AppName>\Parameters
    HKEY hkey;
    TCHAR szKey[1024];
    strcpy_s(szKey, _T("SYSTEM\\CurrentControlSet\\Services\\"));
    strcat_s(szKey, m_szServiceName);
    strcat_s(szKey, _T("\\Parameters"));
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        szKey,
        0,
        KEY_QUERY_VALUE,
        &hkey) == ERROR_SUCCESS) {
        // Yes we are installed
        DWORD dwType = 0;
        DWORD dwSize = sizeof(m_iStartParam);
        RegQueryValueEx(hkey,
                        _T("Start"),
                        NULL,
                        &dwType,
                        (BYTE*) &m_iStartParam,
                        &dwSize);
        dwSize = sizeof(m_iIncParam);
        RegQueryValueEx(hkey,
                        _T("Inc"),
                        NULL,
                        &dwType,
                        (BYTE*) &m_iIncParam,
                        &dwSize);
        RegCloseKey(hkey);
    }

    // Set the initial state
    m_iState = m_iStartParam;

    return TRUE;
}

void CMainService::Run() {
    m_dwThreadId = GetCurrentThreadId();

    if (kCommFaild == m_AppDelegate.Init()) {
        UwlTrace(_T("server initialize failed!"));
        UwlLogFile(_T("server initialize failed!"));
        PostQuitMessage(0);
    }

    MSG msg;
    while (GetMessage(&msg, 0, 0, 0)) {
        DispatchMessage(&msg);
    }

    m_AppDelegate.Term();

    // Sleep for a while
    UwlTrace(_T("service is sleeping to finish(%lu)..."), m_iState);
    Sleep(dwWaitFinished); //wait for any threads to finish

    // Update the current state
    m_iState += m_iIncParam;

}

// Called when the service control manager wants to stop the service
void CMainService::OnStop() {
    UwlTrace(_T("CMainService::OnStop()"));

    PostThreadMessage(m_dwThreadId, WM_QUIT, 0, 0);
}

// Process user control requests
BOOL CMainService::OnUserControl(DWORD dwOpcode) {
    switch (dwOpcode) {
        case SERVICE_CONTROL_USER + 0:

            // Save the current status in the registry
            SaveStatus();
            return TRUE;

        default:
            break;
    }
    return FALSE; // say not handled
}

// Save the current status in the registry
void CMainService::SaveStatus() {
    UwlTrace(_T("Saving current status"));
    // Try opening the registry key:
    // HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\<AppName>\...
    HKEY hkey = NULL;
    TCHAR szKey[1024];
    strcpy_s(szKey, _T("SYSTEM\\CurrentControlSet\\Services\\"));
    strcat_s(szKey, m_szServiceName);
    strcat_s(szKey, _T("\\Status"));
    DWORD dwDisp;
    DWORD dwErr;
    UwlTrace(_T("Creating key: %s"), szKey);
    dwErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           szKey,
                           0,
                           _T(""),
                           REG_OPTION_NON_VOLATILE,
                           KEY_WRITE,
                           NULL,
                           &hkey,
                           &dwDisp);
    if (dwErr != ERROR_SUCCESS) {
        UwlTrace(_T("Failed to create Status key (%lu)"), dwErr);
        return;
    }

    // Set the registry values
    UwlTrace(_T("Saving 'Current' as %ld"), m_iState);
    RegSetValueEx(hkey,
                  _T("Current"),
                  0,
                  REG_DWORD,
                  (BYTE*) &m_iState,
                  sizeof(m_iState));


    // Finished with key
    RegCloseKey(hkey);

}
