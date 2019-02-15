#pragma once
// main.h
#include "resource.h"
class CRabbitMQ;

#define APPLICATION_TITLE	 _T("RobotToolLudo")
#define STR_SERVICE_NAME     _T("RobotToolLudo")
#define STR_DISPLAY_NAME     _T("同城游LUDO机器人工具服务")
#define STR_DISPLAY_NAME_ENU _T("TCY RobotToolLudo Service") 

extern int				g_nClientID;
extern int				g_useLocal;
extern CRobotMgr		g_robotManager; //@zhuhangmin 20181129 为了解决收到的notify太快，丢包的问题

extern std::string		g_curExePath;

extern TCHAR			g_szLicFile[MAX_PATH];
extern TCHAR			g_szIniFile[MAX_PATH];


extern TTokenSockMap	g_mapTokenSock;
extern CCritSec			g_csTokenSock;

extern CSockServer*		g_pSockServer;

extern CSvrInOut  *     g_pSvrInOut;

extern HANDLE			g_hExitServer;

extern UThread			g_thrdTimer;

//////////////////
extern DWORD	GetLocalIPByRemote(LPTSTR szIp, int nPort);
extern SOCKET	FindSocket(const LONG token);
extern void		TokenSockClone(OUT TTokenSockMap& mapTokenSock);

// 通知某个HallSvr
extern BOOL		NotifyOneClient(int nRequest, int nSubReq, int nClientID, int nClientType, void* pData, int nLen);
extern BOOL		NotifyOneClient(UINT nRequest,UINT nSubReq,void* pData, int nLen, LONG token);
extern BOOL		NotifyAllClient(int nRequest, int nSubReq, void* pData, int nLen);

extern CString  ExecHttpRequestPost(const CString& strUrl, const CString& strParams);
