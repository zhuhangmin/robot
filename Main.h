#pragma once

#include "RobotDef.h"

#include "resource.h"
#define APPLICATION_TITLE	 _T("RobotToolLudo")
#define STR_SERVICE_NAME     _T("RobotToolLudo")
#define STR_DISPLAY_NAME     _T("ͬ����LUDO�����˹��߷���")
#define STR_DISPLAY_NAME_ENU _T("TCY RobotToolLudo Service") 

extern int				g_nClientID;
extern int				g_useLocal;
//extern CRobotMgr		g_robotManager; //@zhuhangmin 20181129 Ϊ�˽���յ���notify̫�죬����������

extern std::string		g_curExePath;

extern TCHAR			g_szLicFile[MAX_PATH];
extern TCHAR			g_szIniFile[MAX_PATH];

extern HANDLE			g_hExitServer;

extern UThread			g_thrdTimer;

//////////////////
extern DWORD	GetLocalIPByRemote(LPTSTR szIp, int nPort);

extern CString  ExecHttpRequestPost(const CString& strUrl, const CString& strParams);
