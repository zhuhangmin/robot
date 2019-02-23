#pragma once
#include "server.h"

class CMainService : public CNTService {
public:
    CMainService(const TCHAR* szServiceName, const TCHAR* szDisplayName,
                 const int iMajorVersion, const int iMinorVersion);
    virtual BOOL OnInit();
    virtual void Run();
    virtual void OnStop();
    virtual BOOL OnUserControl(DWORD dwOpcode);

    void SaveStatus();

    int m_iStartParam;
    int m_iIncParam;

    int m_iState;

    CMainServer	m_MainServer;
    DWORD		m_dwThreadId;
};
