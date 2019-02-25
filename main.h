#pragma once

#include "resource.h"
#define APPLICATION_TITLE	 _T("RobotToolLudo")
#define STR_SERVICE_NAME     _T("RobotToolLudo")
#define STR_DISPLAY_NAME     _T("同城游LUDO机器人工具服务")
#define STR_DISPLAY_NAME_ENU _T("TCY RobotToolLudo Service") 

extern int				g_nClientID;

extern std::string		g_curExePath;

extern TCHAR			g_szIniFile[MAX_PATH];

extern HANDLE			g_hExitServer;

//@zhuhangmin 20190225 请勿添加任何具体业务逻辑的全局变量！
