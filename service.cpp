#include "stdafx.h"
#include "service.h"
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
    LOG_INFO("CMainService::OnInit beg");
    HKEY hkey;
    TCHAR szKey[1024];
    strcpy_s(szKey, sizeof(szKey), _T("SYSTEM\\CurrentControlSet\\Services\\"));
    strcat_s(szKey, sizeof(szKey), m_szServiceName);
    strcat_s(szKey, sizeof(szKey), _T("\\Parameters"));
    LOG_INFO("CMainService::OnInit szKey [%s]", szKey);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        szKey,
        0,
        KEY_QUERY_VALUE,
        &hkey) == ERROR_SUCCESS) {
        LOG_INFO("Yes we are installed hkey [%s]", hkey);
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
    LOG_INFO("Set the initial state [%d]", m_iState);
    LOG_INFO("CMainService::OnInit end");
    return TRUE;
}

void CMainService::Run() {
    LOG_INFO("CMainService::Run BEG");
    m_dwThreadId = GetCurrentThreadId();

    if (kCommSucc != m_AppDelegate.Init()) {
        LOG_ERROR("server initialize failed!");
        PostQuitMessage(0);
    }

    MSG msg;
    while (GetMessage(&msg, 0, 0, 0)) {
        DispatchMessage(&msg);
    }

    m_AppDelegate.Term();

    // Sleep for a while
    LOG_INFO("service is sleeping to finish(%lu)...", m_iState);
    Sleep(dwWaitFinished); //wait for any threads to finish

    // Update the current state
    m_iState += m_iIncParam;

    LOG_INFO("CMainService::Run END");

}

// Called when the service control manager wants to stop the service
void CMainService::OnStop() {
    LOG_INFO("CMainService::OnStop()");

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
    LOG_INFO("Saving current status");

    // Try opening the registry key:
    // HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\<AppName>\...
    HKEY hkey = NULL;
    TCHAR szKey[1024];
    strcpy_s(szKey, sizeof(szKey), _T("SYSTEM\\CurrentControlSet\\Services\\"));
    strcat_s(szKey, sizeof(szKey), m_szServiceName);
    strcat_s(szKey, sizeof(szKey), _T("\\Status"));
    DWORD dwDisp;
    DWORD dwErr;
    LOG_INFO("Creating key:  [%s]", szKey);
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
        LOG_ERROR("Failed to create Status key (%lu)", dwErr);
        return;
    }

    // Set the registry values
    LOG_INFO("Saving 'Current' as %ld", m_iState);
    RegSetValueEx(hkey,
                  _T("Current"),
                  0,
                  REG_DWORD,
                  (BYTE*) &m_iState,
                  sizeof(m_iState));


    // Finished with key
    RegCloseKey(hkey);

}
