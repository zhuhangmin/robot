#pragma once
#include "app_delegate.h"

class CMainService : public CNTService {
public:
    CMainService(const TCHAR* szServiceName, const TCHAR* szDisplayName,
                 const int iMajorVersion, const int iMinorVersion);
    BOOL OnInit() override;
    void Run() override;
    void OnStop() override;
    BOOL OnUserControl(DWORD dwOpcode) override;

    void SaveStatus();

    int m_iStartParam;
    int m_iIncParam;

    int m_iState;

    AppDelegate	m_AppDelegate;
    DWORD		m_dwThreadId;
};
