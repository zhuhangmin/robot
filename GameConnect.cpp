#include "xyFrame2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CGameConnect::CGameConnect(const BYTE key[] , const ULONG key_len , DWORD flagEncrypt, DWORD flagCompress )
	: CDefSocketClient(key, key_len, flagEncrypt, flagCompress)
{
	memset(&m_ServerSet,0,sizeof(SERVER_SET));
	memset(&m_ProxySet,0,sizeof(PROXY_SET));
	m_nCommThreadID=0;

	m_dwWaitEchoTicks=0;
	m_dwWaitEchoTimes=0;
	m_dwMaxEchoTicks=0;
}

CGameConnect::~CGameConnect()
{

}
CGameWinApp* CGameConnect::GetGameWinApp()
{
	return (CGameWinApp*)AfxGetApp();
}


void CGameConnect::ClearConnect()
{
	m_ProxySocket.Close();
	Destroy();
}

BOOL CGameConnect::CreateConnect(LPCTSTR lpName,LPCTSTR lpIP,int nPort,BOOL& bProxyFailed)
{
  	CWaitCursor wait;
	Destroy();
	BOOL bConnect;

	if(m_ProxySet.nNeed)//需要代理
	{
 		if (!m_ProxySocket.ConnectEx(lpIP,nPort, 
									m_ProxySet.nSock, m_ProxySet.szHost, m_ProxySet.nPort,
									m_ProxySet.szUserName, m_ProxySet.szPassword, 10))
		{
			bProxyFailed=TRUE;
			return FALSE;
		}
 
		bConnect=Attach(m_ProxySocket.m_hSocket, 0, 
			 m_nCommThreadID,FALSE, GetHelloData(), GetHelloLength() + 1);
	}
	else
	{
 		bConnect=Create(lpIP, nPort, 10, 0, 
	         m_nCommThreadID , FALSE, GetHelloData(), GetHelloLength() + 1);
	}
	
	if(bConnect)//连接成功
	{	 
		m_ServerSet.nPort=nPort;
   		lstrcpy(m_ServerSet.szName,lpName);
		lstrcpy(m_ServerSet.szIP,lpIP);
	}
	else
	{
	 
	}
	return bConnect;
}
void CGameConnect::GetResponseString(CString &sRet, UINT nResponse)
{
	switch(nResponse)//if not succeed
	{
	case UR_REQUEST_UNKNOWN:
	 	sRet=STR_REQUEST_UNKNOWN;
		break;
	case UR_OBJECT_EXIST:
	 	sRet=STR_OBJECT_EXIST;
		break;
 	case UR_OBJECT_NOT_EXIST:
		sRet=STR_OBJECT_NOT_EXIST;
		break;
	case UR_DATABASE_ERROR:
		sRet=STR_PROCESS_FAILED;
		break;
	case GR_HAVE_NO_CHAIR:
		sRet=STR_LOOKON_NOCHAIR;
		break;
	case GR_GAME_NOT_READY:
		sRet=STR_GAME_NOREADY;
		break;
	case GR_INVALID_IDENTITY:
		sRet=STR_VERIFYUSER_FAILED;
		break;
	case GR_RESPONE_ENTER_MATCH_MISMATCH:
		sRet=STR_HARDID_MISMATCH;
		break;
	case GR_HARDID_MISMATCH:
		sRet=STR_HARDID_MISMATCH;
		break;
	case GR_ROOM_TOKENID_MISMATCH:
		sRet=STR_ROOM_TOKENID_MISMATCH;
		break;
	case GR_ROOMTABLECHAIR_MISMATCH:
		sRet=STR_ROOMTABLECHAIR_MISMATCH;
 		break;
	}
}

int  CGameConnect::GetWaitSecondByRequest(UINT nRequest)
{
	int nRet=REQ_TIMEOUT_SECOND;
	switch(nRequest)
	{
    case GR_ENTER_GAME_EX:
	case GR_REPORT_ENTER_MATCH:
		nRet=30;
		break;
 	case GR_GET_VERSION:
		nRet=15;
		break;
	case GR_QUERY_BREAK:
		nRet=5;
		break;
	case GR_GET_WELFARECONFIG:
	case GR_APPLY_BASEWELFARE:
	case GR_APPLY_BASEWELFARE_EX:
		nRet=30;
		break;
	}
	return nRet;
}

BOOL CGameConnect::ProcessRequest(CString& sRet,UINT nRequest, int& nDataLen, VOID *pDataPtr, UINT &nResponse, LPVOID &pRet,BOOL bNeedEcho/*=TRUE*/)
{
	CWaitCursor Wait;

 	if(!IsConnected())
	{
		nResponse=UR_CONNECT_DISABLE;
		sRet=STR_CONNECT_DISABLE;
	 	return FALSE;
	}
	CONTEXT_HEAD	Context;
	REQUEST			Request;
	REQUEST			Response;
	ZeroMemory(&Context,sizeof(CONTEXT_HEAD));
	ZeroMemory(&Request,sizeof(REQUEST));
	ZeroMemory(&Response,sizeof(REQUEST));
	Context.hSocket =m_hSocket;
	Context.lSession = 0;
	Context.bNeedEcho = bNeedEcho;
	Request.head.nRepeated = 0;
	Request.head.nRequest = nRequest;
	Request.nDataLen =nDataLen;
	Request.pDataPtr = pDataPtr;//////////////

	CWaitCursor wt;
	DWORD dwWaitSecond=GetWaitSecondByRequest(nRequest);
   
	DWORD dwTickBegin  = GetTickCount();

    BOOL bTimeOut=FALSE;
	BOOL bResult  = SendRequest(&Context, &Request, &Response,bTimeOut,dwWaitSecond*1000 );
	if(!bResult)///if timeout or disconnect 
	{
		if(bTimeOut)
		{
			nResponse=UR_RESPONSE_TIMEOUT;
			sRet=STR_SENDREQUEST_TIMEOUT;
		}
		else
			sRet=STR_SENDREQUEST_FAILED;
		return FALSE;
	}

	if (bNeedEcho == TRUE)
	{
		DWORD dwTickCost = GetTickCount() - dwTickBegin;
		if(dwTickCost >=0)
		{
			m_dwWaitEchoTimes++;
			m_dwWaitEchoTicks+=dwTickCost;
			
			if (m_dwMaxEchoTicks < dwTickCost)
				m_dwMaxEchoTicks = dwTickCost;
		}
	}

 	nDataLen=Response.nDataLen;
	nResponse=Response.head.nRequest;
 	pRet=Response.pDataPtr;

	if(nResponse==GR_ERROR_INFOMATION_EX)
	{
        sRet.Format(_T("%s"),pRet);
		SAFE_DELETE_ARRAY(pRet);
		nDataLen=0;
		return FALSE;
	}

	GetResponseString(sRet,nResponse);
	return TRUE;
}

 
void CGameConnect::SetCommThreadID(int nThreadID)
{
	m_nCommThreadID=nThreadID;
}
BOOL CGameConnect::InitConnect()
{
	return InitKey(KEY_GAMESVR_2_0, ENCRYPT_AES, 0);
}

void CGameConnect::InitProxySet(LPCTSTR lpFileName)
{
 	m_ProxySet.nNeed =GetPrivateProfileInt ( _T("PROXY"), _T("Need"), 0, lpFileName) ; 
	m_ProxySet.nPort =GetPrivateProfileInt ( _T("PROXY"), _T("Port"),1080,lpFileName) ;
	m_ProxySet.nSock =GetPrivateProfileInt ( _T("PROXY"), _T("Sock"),PROXYTYPE_SOCKS5, lpFileName) ;

	GetPrivateProfileString ( _T("PROXY"),_T("Host"), _T(""),m_ProxySet.szHost, MAX_SERVERIP_LEN, lpFileName) ; 
	GetPrivateProfileString ( _T("PROXY"),_T("User"), _T(""),m_ProxySet.szUserName, 32, lpFileName) ; 
 	GetPrivateProfileString ( _T("PROXY"),_T("pwd"), _T(""),m_ProxySet.szPassword, 32, lpFileName) ; 

}
CString   CGameConnect::GetProxyServerName()
{
	CString sRet;
	sRet=m_ProxySet.szHost;
	return sRet;
}

 
BOOL CGameConnect::GC_CheckVersion(CString& sRet,BOOL& bDownLoad,BOOL& bUpdateExeFile)
{
	CWaitCursor wait;

    VERSION Version;
	memset(&Version,0,sizeof(VERSION));

	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=0;
  	BOOL bResult = ProcessRequest(sRet,GR_GET_VERSION, nDataLen, 0, nResponse, pRet);
  	if(!bResult || pRet==NULL)//
	{
		sRet.Format(_T("%s"),STR_SERVER_BUSY);
  		return FALSE;
	}
	/////////////////////////////////////////////////////////////////
  	memcpy(&Version,pRet,sizeof(VERSION));
	SAFE_DELETE_ARRAY(pRet);	
  
	int nVerMajor=GetGameWinApp()->GetVerMajor();
	int nVerMinor=GetGameWinApp()->GetVerMinor();
   	if(Version.nMajorVer==nVerMajor && Version.nMinorVer ==nVerMinor )
  		return TRUE;
	
	if(Version.nMajorVer>nVerMajor )//当前版本太旧，无法用
	{
		sRet.Format(_T("%s"),STR_VERSION_DISUSE);
		bDownLoad=Version.nMajorVer;
		return FALSE;
	}
	else if(Version.nMajorVer<nVerMajor )//当前版本太新
	{
		sRet.Format(_T("%s"),STR_SERVER_UPDATE);
  		return FALSE;
	}
	if(Version.nMajorVer==nVerMajor )
	{
		if(Version.nMinorVer >nVerMinor)//当前版本旧了
		{
			sRet.Format(_T("%s"),STR_EXEFILE_NEEDUPDATE);
 			bUpdateExeFile=Version.nMinorVer;
 			return FALSE;
		}
		else if(Version.nMinorVer<nVerMinor)//当前版本太新
		{
			sRet.Format(_T("%s"),STR_SERVER_UPDATE);
 			return FALSE;
		}
	}
 	return FALSE;
}
BOOL  CGameConnect::GC_GetActiveConfig()
{
	ACTIVECONFIG config;
	memset(&config,0,sizeof(ACTIVECONFIG));
	
	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=0;
	CString sRet;
	BOOL bResult = ProcessRequest(sRet,GR_GET_ACTIVE_CONFIG, nDataLen, 0, nResponse, pRet);
	if(!bResult || pRet==NULL)//
	{
		sRet.Format(_T("%s"),STR_SERVER_BUSY);
		return FALSE;
	}
	memcpy(&config,pRet,sizeof(ACTIVECONFIG));
	GetGameWinApp()->m_bBetaNet = config.isBetaNet!=0;
	CString urlP = config.szActiveURLs;

	int pos = 0;
    CString strCur = "";
	std::vector<CString> params;
    while(AfxExtractSubString(strCur, config.szActiveURLs, pos, '|'))
    {
        ++pos;
		params.push_back(strCur);
    }
	
	UwlLogFile(_T("ACTIVECONFIG count=[%d]"),config.nCount);
	for (int n = 0; n < config.nCount; n++)
	{
		if (config.nActives[n]<0 || config.nActives[n] >= GetGameWinApp()->m_vecActivePics.size())
			continue;
		UwlLogFile(_T("ACTIVECONFIG [%d]=%s"),n,params[n]);
		GetGameWinApp()->m_vecActiveOrder.push_back(config.nActives[n]);
		// 解析出 w h u
		UnitActiveParam u;
		u.nHeight = u.nWidth =0;
		pos = params[n].Find("!",0);
		if (pos >= 0 && pos != params[n].GetLength())
			u.url = params[n].Right(params[n].GetLength() - pos -1);

		if( pos > 0 ){
			CString strWH =	params[n].Left(pos);
			pos = strWH.Find("x",0);
			if( pos >0 ){
				u.nWidth  = _ttoi(strWH.Left(pos));
				u.nHeight = _ttoi(strWH.Right(strWH.GetLength() - pos - 1));
			}
		}
		GetGameWinApp()->m_mapActiveUrlP[config.nActives[n]] = u;
	}
	SAFE_DELETE_ARRAY(pRet);	
	return TRUE;
}
BOOL  CGameConnect::FillEnterGameEx(ENTER_GAME_EX* pEGE)
{
	GAMEPLAYER* pGamePlayer=GetGameWinApp()->GetGamePlayerByUserID(GetGameWinApp()->GetUserID());
	if(pGamePlayer==NULL)
   		return FALSE;

	pEGE->nUserID =pGamePlayer->nUserID;
	pEGE->nUserType=pGamePlayer->nUserType;

	pEGE->nGameID=GetGameWinApp()->GetGameID();
	pEGE->nRoomID=GetGameWinApp()->GetRoomID();
	pEGE->nTableNO=GetGameWinApp()->GetTableNO();
	pEGE->nChairNO =GetGameWinApp()->GetChairNO();


    xyGetHardID(pEGE->szHardID) ;
	
	pEGE->bLookOn=GetGameWinApp()->IsLookOn();

	pEGE->dwUserConfigs=GetGameWinApp()->GetUserConfig();
	pEGE->nRoomTokenID=GetGameWinApp()->GetRoomTokenID();


	return TRUE;
}

BOOL  CGameConnect::FillSoloPlayer(SOLO_PLAYER* pSP)
{
	GAMEPLAYER* pGamePlayer=GetGameWinApp()->GetGamePlayerByUserID(GetGameWinApp()->GetUserID());
	if(pGamePlayer==NULL)
		return FALSE;

	GetGameWinApp()->SetSoloPlayerByGamePlayer(pSP,pGamePlayer);
	
	return TRUE;
}

BOOL  CGameConnect::FillEnterMatchEx(ENTER_MATCH* pEME)
{
	
	GAMEPLAYER* pGamePlayer=GetGameWinApp()->GetGamePlayerByUserID(GetGameWinApp()->GetUserID());
	if(pGamePlayer==NULL)
		return FALSE;
	
	pEME->nGameID=GetGameWinApp()->GetGameID();
	pEME->nMatchID=GetGameWinApp()->GetMatchID();
	pEME->nUserID =pGamePlayer->nUserID;
	pEME->nUserType=pGamePlayer->nUserType;
	pEME->nRoomID=GetGameWinApp()->GetRoomID();
	pEME->nTableNO=GetGameWinApp()->GetTableNO();
	pEME->nChairNO =GetGameWinApp()->GetChairNO();
	pEME->bLookOn=GetGameWinApp()->IsLookOn();
    xyGetHardID(pEME->szHardID) ;
	pEME->nRoomPort =GetGameWinApp()->GetRoomPort();
	pEME->nRoomTokenID=GetGameWinApp()->GetRoomTokenID();

	return TRUE;
}
void CGameConnect::PR_EnterGameOK(LPVOID pRet,int nDataLen)
{
	if(nDataLen>0)
   		memcpy(GetGameWinApp()->GetEnterInfo(),(BYTE*)pRet ,nDataLen);

	//GetEnterInfoSize
}

void  CGameConnect::PR_EnterGameDXXW(LPVOID pRet,int nDataLen)
{
	if(nDataLen>0)
	{
		if (GetGameWinApp()->IsCloakingRoom() && !GetGameWinApp()->IsLookOn())
		{
			SOLOPLAYER_HEAD* pSoloPlayerHead = (SOLOPLAYER_HEAD*)(BYTE*)pRet;
			GetGameWinApp()->ResetDataBySoloTableDXXW(pSoloPlayerHead);

			int nPlayerSize = sizeof(SOLOPLAYER_HEAD)+pSoloPlayerHead->nPlayerCount*sizeof(SOLO_PLAYER);
			memcpy(GetGameWinApp()->GetTableInfo(),(BYTE*)pRet+nPlayerSize,nDataLen-nPlayerSize);
		}
		else
		{
			memcpy(GetGameWinApp()->GetTableInfo(),(BYTE*)pRet ,nDataLen);
		}
	}
}

//空闲玩家进入游戏
void  CGameConnect::PR_EnterGameIDLE(LPVOID pRet,int nDataLen)
{
	if(nDataLen>0)
		memcpy(GetGameWinApp()->GetTableInfo(),(BYTE*)pRet ,nDataLen);
}

void ActiveWnd(HWND hWnd)
{
	::SetForegroundWindow(hWnd);
	if(::IsIconic(hWnd))
		::ShowWindow(hWnd,SW_SHOWMAXIMIZED);
	
	::InvalidateRect(hWnd,NULL,TRUE);
	::UpdateWindow(hWnd);
}

BOOL CGameConnect::GC_EnterGame(CString& sRet)
{
	ENTER_GAME_EX ege;
	memset(&ege,0,sizeof(ege));

	SOLO_PLAYER sp;
	memset(&sp,0,sizeof(sp));

	FillEnterGameEx(&ege);
	FillSoloPlayer(&sp);

 	if(ege.nUserID==0)
	{
		sRet.Format(_T("%s"),STR_OBJECT_NOT_EXIST);
		return FALSE;
	}
	///////////     Process  Request   ///////////////////////////////////////
	UINT nResponse=0;
	LPVOID pRet=NULL;
	
	int nDataLen=sizeof(ENTER_GAME_EX)+sizeof(SOLO_PLAYER);
	BYTE * pSendData=new BYTE[nDataLen];
	memset(pSendData,0,nDataLen);
	memcpy(pSendData,&ege,sizeof(ege));
	memcpy(pSendData+sizeof(ege),&sp,sizeof(sp));
	if(!GetGameWinApp()->IsSoloRoom())
		nDataLen=sizeof(ENTER_GAME_EX);

	BOOL bResult = ProcessRequest(sRet,GR_ENTER_GAME_EX, nDataLen,pSendData, nResponse, pRet);
	SAFE_DELETE(pSendData);
	if(!bResult)//
	{
		//sRet="服务器未回应登入请求!";
		SAFE_DELETE_ARRAY(pRet);	
		return FALSE;
	}

	if(nResponse==GR_RESPONE_ENTER_GAME_OK) 
	{
	    GetGameWinApp()->SetDXXW(FALSE);
		PR_EnterGameOK(pRet,nDataLen);
		GetGameWinApp()->OnEnterGameOK(pRet,nDataLen);
		SAFE_DELETE_ARRAY(pRet);	
		return TRUE;
	}
	else if (nResponse==GR_RESPONE_ENTER_GAME_DXXW)
	{
		GetGameWinApp()->SetDXXW(!GetGameWinApp()->IsLookOn());
		PR_EnterGameDXXW(pRet,nDataLen);
		GetGameWinApp()->OnEnterGameDXXW(pRet,nDataLen);
		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}
	else if (nResponse==GR_RESPONE_ENTER_GAME_IDLE)		//游戏中，空闲玩家进入游戏
	{
		GetGameWinApp()->SetDXXW(FALSE);
		PR_EnterGameIDLE(pRet,nDataLen);
		GetGameWinApp()->OnEnterGameIDLE(pRet,nDataLen);
		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}
	else if (nResponse==GR_WAIT_CHECKRESULT)
	{
		sRet=STR_ENTER_WAIT_CHECKRESULT;
	}

	SAFE_DELETE_ARRAY(pRet);	
   	return FALSE;
}

BOOL CGameConnect::GC_EnterMatch(CString& sRet)
{
  	ENTER_MATCH ege;
	memset(&ege,0,sizeof(ENTER_MATCH));
	FillEnterMatchEx(&ege);
 	if(ege.nUserID==0)
	{
		sRet.Format(_T("%s"),STR_OBJECT_NOT_EXIST);
 		return FALSE;
	}
  	///////////     Process  Request   ///////////////////////////////////////
 	UINT nResponse=0;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(ENTER_MATCH);
	BOOL bResult = ProcessRequest(sRet,GR_REPORT_ENTER_MATCH, nDataLen, &ege, nResponse, pRet);

 	if(!bResult)//
	{
		sRet="服务器未回应登入请求!";
		return FALSE;
	}
	



//	ActiveWnd(AfxGetMainWnd()->m_hWnd);
  	if(nResponse==GR_RESPONE_ENTER_MATCH_OK) 
	{
		GetGameWinApp()->OnEnterMatchOK(pRet,nDataLen);
		SAFE_DELETE_ARRAY(pRet);	
		return TRUE;
	}
	else if (nResponse==GR_RESPONE_ENTER_MATCH_WASHOUT)
	{
		GetGameWinApp()->OnEnterMatchWashOut(pRet,nDataLen);
		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}
	else if (nResponse==GR_RESPONE_ENTER_MATCH_FINISHED)
	{
		GetGameWinApp()->OnEnterMatchFinished(pRet,nDataLen);
		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}
	else if (nResponse==GR_RESPONE_ENTER_MATCH_NOSIGNUP)
	{
		GetGameWinApp()->OnEnterMatchNoSignUp(pRet,nDataLen);
		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}
	else if (nResponse==GR_RESPONE_ENTER_MATCH_LATE)
	{
		GetGameWinApp()->OnEnterMatchLated(pRet,nDataLen);
		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}
	else if (nResponse==GR_RESPONE_ENTER_MATCH_STARTED)
	{
		GetGameWinApp()->OnEnterMatchStarted(pRet,nDataLen);
		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}
	else if (nResponse==GR_RESPONE_ENTER_MATCH_SYSTEMCANCEL)
	{
		GetGameWinApp()->OnEnterMatchSystemCancel(pRet,nDataLen);
		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}
	else if (nResponse==GR_RESPONE_ENTER_MATCH_SELFCANCEL)
	{
		GetGameWinApp()->OnEnterMatchSelfCancel(pRet,nDataLen);
		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}
	else if (nResponse==GR_RESPONE_ENTER_MATCH_SIGNUPUNCHECK)
	{
		GetGameWinApp()->OnEnterMatchSignUpUnCheck(pRet,nDataLen);
		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}
	else if (nResponse==GR_RESPONE_ENTER_MATCH_REJECTED)
	{
		GetGameWinApp()->OnEnterMatchRejected(pRet,nDataLen);
		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}
	else if (nResponse==GR_RESPONE_ENTER_MATCH_ERROR)
	{
		//
		GetGameWinApp()->OnEnterMatchError((LPCTSTR)pRet);
		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}


    SAFE_DELETE_ARRAY(pRet);	
   	return FALSE;
}

BOOL  CGameConnect::GC_MatchPlayerOK()
{
	MATCH_PLAYER_OK player_ok;
	memset(&player_ok,0,sizeof(MATCH_PLAYER_OK));
	player_ok.nUserID=GetGameWinApp()->GetUserID();
	player_ok.nMatchID=GetGameWinApp()->GetMatchID();
	player_ok.nRoomID=GetGameWinApp()->GetRoomID();
	player_ok.nRoomTokenID=GetGameWinApp()->GetRoomTokenID();

	UINT nResponse=0;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(MATCH_PLAYER_OK);
	CString sRet;
	BOOL bResult = ProcessRequest(sRet,GR_REPORT_MATCHPLAYER_OK, nDataLen, &player_ok, nResponse, pRet,FALSE);
	if (!bResult)
		return FALSE;

	return TRUE;
}

BOOL CGameConnect::GC_LeaveGame(int nLeaveUser,int nLeaveChair)
{
	CString sRet; 
	LEAVE_GAME lg;
	memset(&lg,0,sizeof(LEAVE_GAME));
	lg.sender_info.nSendTable=GetGameWinApp()->GetTableNO();
	lg.sender_info.nSendChair=GetGameWinApp()->GetChairNO();
	lg.sender_info.nSendUser=GetGameWinApp()->GetUserID();
    xyGetHardID(lg.sender_info.szHardID) ; //硬件ID

	lg.nRoomID =GetGameWinApp()->GetRoomID();
	lg.nTableNO =GetGameWinApp()->GetTableNO();

 	lg.nUserID =nLeaveUser;
	lg.nChairNO =nLeaveChair;
	lg.bPassive =(nLeaveUser==lg.sender_info.nSendUser ? FALSE:TRUE);
 
 	///////////     Process  Request   ///////////////////////////////////////
	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(LEAVE_GAME);
	BOOL bResult = ProcessRequest(sRet,GR_LEAVE_GAME, nDataLen, &lg, nResponse, pRet,FALSE);
   	return bResult;
}

BOOL CGameConnect::GC_LeaveGameEx(CString& sRet,int nLeaveUser,int nLeaveChair)
{
	sRet.Empty();
	
	LEAVE_GAME lg;
	memset(&lg,0,sizeof(LEAVE_GAME));
	lg.sender_info.nSendTable=GetGameWinApp()->GetTableNO();
	lg.sender_info.nSendChair=GetGameWinApp()->GetChairNO();
	lg.sender_info.nSendUser=GetGameWinApp()->GetUserID();
    xyGetHardID(lg.sender_info.szHardID) ; //硬件ID
	
	lg.nRoomID =GetGameWinApp()->GetRoomID();
	lg.nTableNO =GetGameWinApp()->GetTableNO();
	
	lg.nUserID =nLeaveUser;
	lg.nChairNO =nLeaveChair;
	lg.bPassive =(nLeaveUser==lg.sender_info.nSendUser ? FALSE:TRUE);
	
	///////////     Process  Request   ///////////////////////////////////////
	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(LEAVE_GAME);
	BOOL bResult = ProcessRequest(sRet,GR_LEAVE_GAME_EX, nDataLen, &lg, nResponse, pRet);
	if(!bResult)//
		return FALSE;
	if(nResponse==GR_LEAVEGAME_PLAYING)
	{
		sRet=STR_FORBID_DESERT;
		return TRUE;
	}
	if(nResponse==GR_LEAVEGAME_TOOFAST)
	{
		int nSeconds=*(int*)pRet;
		sRet.Format(STR_NEED_WAIT_FOR_RANDOM,nSeconds);
		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}
	if (nResponse==GR_WAIT_CHECKRESULT)
	{
		sRet=STR_LEAVE_WAIT_CHECKRESULT;
		return TRUE;
	}
    return FALSE;
}

BOOL  CGameConnect::GC_SendGamePulse()
{
	GAME_PULSE game_pulse;
	memset(&game_pulse,0,sizeof(GAME_PULSE));
	
	game_pulse.nUserID=GetGameWinApp()->GetUserID();
	
	if (m_dwWaitEchoTimes>0)//在一分钟内有发送过等待回应的请求
	{
		game_pulse.dwAveDelay = m_dwWaitEchoTicks / m_dwWaitEchoTimes;
		game_pulse.dwMaxDelay = m_dwMaxEchoTicks;
		
		//延迟最大不超过60秒
		if (game_pulse.dwAveDelay > 60000) game_pulse.dwAveDelay = 60000;
		if (game_pulse.dwMaxDelay > 60000) game_pulse.dwMaxDelay = 60000;
		
	}
	
	m_dwWaitEchoTicks=0;
	m_dwWaitEchoTimes=0;
	m_dwMaxEchoTicks=0;

	CString sRet;
	UINT nResponse=0;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(GAME_PULSE);
	BOOL bResult = ProcessRequest(sRet,GR_GAME_PULSE, nDataLen, &game_pulse, nResponse, pRet,FALSE);
	return bResult;
}

BOOL CGameConnect::GC_QueryBreak()
{
	CString sRet; 
	QUERY_BREAK qb;
	memset(&qb,0,sizeof(QUERY_BREAK));
 	qb.nUserID =GetGameWinApp()->GetUserID();
	qb.nRoomID =GetGameWinApp()->GetRoomID();
	qb.nTableNO =GetGameWinApp()->GetTableNO();
	qb.nChairNO =GetGameWinApp()->GetChairNO();

 	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof( QUERY_BREAK);

	BOOL bResult = ProcessRequest(sRet,GR_QUERY_BREAK, nDataLen, &qb, nResponse, pRet);
	if(!bResult)//
		return FALSE;

	if(pRet!=NULL)
	{
		DWORD dwRoomOptions=*((DWORD*)pRet);
		GetGameWinApp()->m_GameRoom.dwOptions =dwRoomOptions;
		
		SAFE_DELETE_ARRAY(pRet);
	}
	
    if(nResponse==GR_BREAK_CUT_YES )///if succeed	
 		return TRUE;
   	return FALSE;
}
BOOL  CGameConnect::GC_GiveupGame(BOOL bTimeOut)
{	
	CString sRet; 

	GIVE_UP_GAME gug;
	memset(&gug,0,sizeof(gug));
	gug.nUserID =GetGameWinApp()->GetUserID();
	gug.nRoomID =GetGameWinApp()->GetRoomID();
	gug.nTableNO =GetGameWinApp()->GetTableNO();
	gug.nChairNO =GetGameWinApp()->GetChairNO();
	gug.bTimeOut=bTimeOut;

	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(GIVE_UP_GAME);
	BOOL bResult = ProcessRequest(sRet,GR_GIVE_UP_GAME, nDataLen, &gug, nResponse, pRet,FALSE);
  	return bResult;
}
BOOL  CGameConnect::GC_CallPlayer(int nPlayerID)
{	
	CString sRet;

	CALL_PLAYER  cp;
	memset(&cp,0,sizeof(CALL_PLAYER));

 	cp.nUserID =GetGameWinApp()->GetUserID();
	cp.nRoomID =GetGameWinApp()->GetRoomID();
	cp.nTableNO =GetGameWinApp()->GetTableNO();
	cp.nChairNO =GetGameWinApp()->GetChairNO();
 	cp.nCalleeID =nPlayerID;
	cp.nCalleeNO=GetGameWinApp()->GetChairNOByUserID(nPlayerID);

	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(CALL_PLAYER);
	BOOL bResult = ProcessRequest(sRet,GR_CALL_PLAYER, nDataLen, &cp, nResponse, pRet,FALSE);
  	return bResult;
}

BOOL CGameConnect::GC_GetTableInfo()
{
	GET_TABLE_INFO  gti;
	memset(&gti,0,sizeof(GET_TABLE_INFO));
	gti.nUserID=GetGameWinApp()->GetUserID();
	gti.nRoomID=GetGameWinApp()->GetRoomID();
	gti.nTableNO=GetGameWinApp()->GetTableNO();
	gti.nChairNO=GetGameWinApp()->GetChairNO();

  	CString  sRet;
 	//////////     Process  Request   ///////////////////////////////////////
	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(GET_TABLE_INFO);
	BOOL bResult = ProcessRequest(sRet,GR_GET_TABLE_INFO, nDataLen, &gti, nResponse, pRet);
	if(!bResult)//
		return FALSE;
 	if(pRet && nResponse==UR_OPERATE_SUCCEEDED)
	{
		memcpy(GetGameWinApp()->GetTableInfo(),pRet,nDataLen);
   		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}
  	return FALSE;
}

BOOL CGameConnect::GC_StartGame(CString& sRet)
{
 	START_GAME sg;
	memset(&sg,0,sizeof(START_GAME));
 	sg.nUserID =GetGameWinApp()->GetUserID();
	sg.nRoomID =GetGameWinApp()->GetRoomID();
	sg.nTableNO =GetGameWinApp()->GetTableNO();
	sg.nChairNO =GetGameWinApp()->GetChairNO();
 	sRet.Empty();

  	///////////     Process  Request   ///////////////////////////////////////
 	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(START_GAME);
	BOOL bResult = ProcessRequest(sRet,GR_START_GAME, nDataLen,&sg, nResponse, pRet,FALSE);
    return bResult;
}

BOOL CGameConnect::GC_StartGameEx(CString& sRet)
{
	START_GAME sg;
	memset(&sg,0,sizeof(START_GAME));
	sg.nUserID =GetGameWinApp()->GetUserID();
	sg.nRoomID =GetGameWinApp()->GetRoomID();
	sg.nTableNO =GetGameWinApp()->GetTableNO();
	sg.nChairNO =GetGameWinApp()->GetChairNO();
	sRet.Empty();
	
	///////////     Process  Request   ///////////////////////////////////////
	UINT nResponse=0;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(START_GAME);
	BOOL bResult = ProcessRequest(sRet,GR_START_GAME_EX, nDataLen,&sg, nResponse, pRet);

	if (!bResult)
	{
		return FALSE;
	}

	if(nResponse==GR_RESPONE_DEPOSIT_NOTENOUGH) 
	{
		GetGameWinApp()->OnDepositNotEnough(pRet, nDataLen);
		SAFE_DELETE_ARRAY(pRet);	
		return FALSE;
	}
	else if (nResponse==GR_RESPONE_DEPOSIT_TOOHIGH)
	{
		GetGameWinApp()->OnDepositTooHigh(pRet,nDataLen);
		SAFE_DELETE_ARRAY(pRet);
		return FALSE;
	}
	else if (nResponse==UR_OPERATE_SUCCEEDED)
	{
		SAFE_DELETE_ARRAY(pRet);
		CMainGame* pMainGame=(CMainGame*)GetGameWinApp()->GetMainGame();
		if (pMainGame)
		{
			pMainGame->OPE_StopEventKickOff();	
		}
		
   		return TRUE;
	}

	SAFE_DELETE_ARRAY(pRet);	
   	return FALSE;
}

BOOL  CGameConnect::GC_AutoStartGame(CString& sRet)
{
	START_GAME sg;
	memset(&sg,0,sizeof(START_GAME));
	sg.nUserID =GetGameWinApp()->GetUserID();
	sg.nRoomID =GetGameWinApp()->GetRoomID();
	sg.nTableNO =GetGameWinApp()->GetTableNO();
	sg.nChairNO =GetGameWinApp()->GetChairNO();
	sRet.Empty();
	
	///////////     Process  Request   ///////////////////////////////////////
	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(START_GAME);
	BOOL bResult = ProcessRequest(sRet,GR_AUTO_START_GAME, nDataLen,&sg, nResponse, pRet,FALSE);
    return bResult;
}


BOOL CGameConnect::GC_ChatToTable(LPCTSTR lpszMsg)
{
  	CString sRet;

	CHAT_TO_TABLE  ctt;
	memset(&ctt,0,sizeof(CHAT_TO_TABLE));
    xyGetHardID(ctt.szHardID) ; //硬件ID

 	ctt.nUserID =GetGameWinApp()->GetUserID();
	ctt.nRoomID =GetGameWinApp()->GetRoomID();
	ctt.nTableNO =GetGameWinApp()->GetTableNO();
	ctt.nChairNO =GetGameWinApp()->GetChairNO();
 	lstrcpyn(ctt.szChatMsg,lpszMsg,MAX_CHATMSG_LEN-2); 

 	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(CHAT_TO_TABLE);
	BOOL bResult = ProcessRequest(sRet,GR_CHAT_TO_TABLE, nDataLen, &ctt, nResponse, pRet,FALSE);
  	return bResult;

}

BOOL CGameConnect::GC_ChatToTableEx(LPCTSTR lpszMsg)
{
	CString sRet;
	
	CHAT_TO_TABLE  ctt;
	memset(&ctt,0,sizeof(CHAT_TO_TABLE));
    xyGetHardID(ctt.szHardID) ; //硬件ID
	
	ctt.nUserID =GetGameWinApp()->GetUserID();
	ctt.nRoomID =GetGameWinApp()->GetRoomID();
	ctt.nTableNO =GetGameWinApp()->GetTableNO();
	ctt.nChairNO =GetGameWinApp()->GetChairNO();
	lstrcpyn(ctt.szChatMsg,lpszMsg,MAX_CHATMSG_LEN-2); 
	
	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(CHAT_TO_TABLE);
	BOOL bResult = ProcessRequest(sRet,GR_CHAT_TO_TABLE_EX, nDataLen, &ctt, nResponse, pRet,FALSE);
	return bResult;
}

BOOL  CGameConnect::GC_ClientCrash(int nRequestID,VOID* pDataPtr,int nSize)
{
	CString sRet;

	CLIENT_CRASH crash;
	memset(&crash,0,sizeof(CLIENT_CRASH));
	crash.nUserID=GetGameWinApp()->GetUserID();
	crash.nRoomID=GetGameWinApp()->GetRoomID();
	crash.nTableNO=GetGameWinApp()->GetTableNO();
	crash.nChairNO=GetGameWinApp()->GetTableNO();
	crash.bLookOn=GetGameWinApp()->IsLookOn();

	crash.nCrashRequest=nRequestID;
	crash.nSubRequest=0;
	crash.nCrashDataSize=nSize;
	crash.dwRequestTick=GetTickCount();

	crash.nLastRequest=GetGameWinApp()->GetGameNotify()->m_nLastRequestID;
	crash.nLastSubRequest=GetGameWinApp()->GetGameNotify()->m_nLastSubRequestID;
	crash.nLastDataSize=GetGameWinApp()->GetGameNotify()->m_nLastDataSize;
	crash.dwLastRequestTick=GetGameWinApp()->GetGameNotify()->m_dwLastRequestTick;

	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(CLIENT_CRASH);
	BOOL bResult = ProcessRequest(sRet,GR_CLIENT_CRASH, nDataLen, &crash, nResponse, pRet,FALSE);
  	return bResult;
}

BOOL  CGameConnect::GC_GetUserDetail(int nUserID)
{
// 	if (nUserID==GetGameWinApp()->GetUserID()
// 		&&GetGameWinApp()->IsMatchPlaying())
// 	{
// 		GAMEPLAYER* pPlayer=GetGameWinApp()->GetGamePlayerByUserID(nUserID);
// 		if (!pPlayer) return FALSE;
// 
// 		MATCH_USERDETAIL mu;
// 		memset(&mu,0,sizeof(MATCH_USERDETAIL));
// 		strcpy(mu.szNickName,pPlayer->szNickName);
// 		strcpy(mu.szUsername,pPlayer->szUsername);
// 		mu.nNickSex=pPlayer->nNickSex;
// 		mu.nPortrait=pPlayer->nPortrait;
// 		mu.nClothingID=pPlayer->nClothingID;
// 		mu.nUserType=pPlayer->match_player.nUserType;
// 
// 		mu.nDuan=pPlayer->nDuan;
// 		mu.nDuanScore=playinfo.userongame.nScore;
// 		mu.nDuanNeedScore=playinfo.nDuanNeedScore;
// 
// 		mu.nRank=playinfo.useronmatch.nRank;
// 		mu.nScore=playinfo.useronmatch.nScore;
// 		mu.nDeposite=playinfo.useronmatch.nDeposit;
// 		mu.nWin=playinfo.useronmatch.nWin;
// 		mu.nLoss=playinfo.useronmatch.nLoss;
// 		mu.nStandoff=playinfo.useronmatch.nStandOff;
// 		mu.nBout=playinfo.useronmatch.nBout;
// 		mu.nBreakOff=playinfo.useronmatch.nBreakOff;
// 
// 		mu.nTotalWin=playinfo.userongame.nWin;
// 		mu.nTotalLoss=playinfo.userongame.nLoss;
// 		mu.nTotalStandoff=playinfo.userongame.nStandOff;
// 		mu.nTotalBout=playinfo.userongame.nBout;
// 		mu.nTotalBreakOff=playinfo.userongame.nBreakOff;
// 
// 		GetGameWinApp()->GetGameFrame()->NTF_UserDetail(&mu);
// 		return TRUE;
// 	}
// 	else
	{
		CString sRet;
		UINT nResponse;
		LPVOID pRet=NULL;
		MATCH_GETUSERDETAIL mg;
		memset(&mg,0,sizeof(MATCH_GETUSERDETAIL));
		mg.nRoomID=GetGameWinApp()->GetRoomID();
		mg.nReqUserID=GetGameWinApp()->GetUserID();
		mg.nDestUserID=nUserID;
		int nDataLen=sizeof(MATCH_GETUSERDETAIL);
		BOOL bResult = ProcessRequest(sRet,GR_REPORT_GETUSERDETAIL,nDataLen , &mg, nResponse, pRet,FALSE);
		return bResult;
	}
}

BOOL  CGameConnect::GC_SignUp()
{
	CString sRet;
	UINT nResponse;
	LPVOID pRet=NULL;
	int nLen=sizeof(MATCH_ASKSIGNUP);

	MATCH_ASKSIGNUP ma;
	memset(&ma,0,sizeof(MATCH_ASKSIGNUP));
	ma.nRoomID=GetGameWinApp()->GetRoomID();
	ma.nUserID=GetGameWinApp()->GetUserID();
	ma.nGameID=GetGameWinApp()->GetGameID();

	BOOL bResult = ProcessRequest(sRet,GR_REPORT_SIGNUP,nLen, &ma, nResponse, pRet,FALSE);

	return bResult;
}

BOOL  CGameConnect::GC_GiveUp()
{
	CString sRet;
	UINT nResponse;
	LPVOID pRet=NULL;
	int nLen=sizeof(MATCH_ASKGIVEUP);

	MATCH_ASKGIVEUP ma;
	memset(&ma,0,sizeof(MATCH_ASKGIVEUP));
	ma.nRoomID=GetGameWinApp()->GetRoomID();
	ma.nUserID=GetGameWinApp()->GetUserID();
	ma.nGameID=GetGameWinApp()->GetGameID();

	BOOL bResult = ProcessRequest(sRet,GR_REPORT_GIVEUP,nLen , &ma, nResponse, pRet,FALSE);
	
	return bResult;
}



BOOL  CGameConnect::REQ_AskExit(CString sReason)
{
	ASK_EXIT  aexit;
	memset(&aexit,0,sizeof(ASK_EXIT));
	aexit.nUserID   	=		GetGameWinApp()->GetUserID();
	aexit.nRoomID	    =		GetGameWinApp()->GetRoomID();
	aexit.nTableNO	    =		GetGameWinApp()->GetTableNO();
	aexit.nChairNO	    =		GetGameWinApp()->GetChairNO();	
	memcpy(aexit.sReason,sReason,sizeof(aexit.sReason));
	
	CString sRet;
	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(ASK_EXIT);
	BOOL bResult = ProcessRequest(sRet,GR_ASK_EXIT, nDataLen, &aexit, nResponse, pRet,FALSE);
   	return bResult;
}

BOOL  CGameConnect::REQ_AllowExit(int nAskID,BOOL bAllowed)
{
	ALLOW_EXIT  aaexit;
	memset(&aaexit,0,sizeof(ALLOW_EXIT));
	aaexit.nUserID=GetGameWinApp()->GetUserID();
	aaexit.nRoomID=GetGameWinApp()->GetRoomID();
	aaexit.nTableNO=GetGameWinApp()->GetTableNO();
	aaexit.nChairNO=GetGameWinApp()->GetChairNO();
	aaexit.nAskerID=nAskID;
	aaexit.bAllowed=bAllowed;
	///////////     Process  Request   ///////////////////////////////////////
	CString sRet;
	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(ALLOW_EXIT);
	BOOL bResult = ProcessRequest(sRet,GR_ALLOW_EXIT, nDataLen, &aaexit, nResponse, pRet,FALSE);
   	return bResult;
}

BOOL  CGameConnect::GC_AllowCardView(int nLookerID,BOOL bAllowed)
{
	ALLOW_CARD_VIEW  acv;
	memset(&acv,0,sizeof(ALLOW_CARD_VIEW));
	acv.nUserID=GetGameWinApp()->GetUserID();
	acv.nRoomID=GetGameWinApp()->GetRoomID();
	acv.nTableNO=GetGameWinApp()->GetTableNO();
	acv.nChairNO=GetGameWinApp()->GetChairNO();
	acv.nLookerID=nLookerID;
	acv.bAllowed=bAllowed;
	///////////     Process  Request   ///////////////////////////////////////
	CString sRet;
	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(ALLOW_CARD_VIEW);
	BOOL bResult = ProcessRequest(sRet,GR_ALLOW_CARD_VIEW, nDataLen, &acv, nResponse, pRet,FALSE);
   	return bResult;
}

BOOL CGameConnect::GC_DrawLottery()
{
	LOTTERY_DRAW ld;
	ZeroMemory(&ld, sizeof(ld));
	ld.nUserID  = GetGameWinApp()->GetUserID();
	ld.nRoomID  = GetGameWinApp()->GetRoomID();
	ld.nTableNO = GetGameWinApp()->GetTableNO();
	ld.nChairNO = GetGameWinApp()->GetChairNO();
	//ld.nActID
	//lstrcpy(ld.szGameCode, APP_NAME);
	lstrcpy(ld.szUserName, GetGameWinApp()->GetUserNameByID(ld.nUserID));
	xyGetHardID(ld.szHardID);
	ld.bIsTrunk = FALSE; //ios用户填TRUE;
	
	CString sRet;
	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(LOTTERY_DRAW);
	
	ProcessRequest(sRet, GR_LOTTERY_DRAW, nDataLen, &ld, nResponse, pRet, FALSE);
	
	return TRUE;
}

BOOL CGameConnect::GC_DrawLotryToGame()
{
	LOTTERY_DRAW ld;
	ZeroMemory(&ld, sizeof(ld));
	ld.nUserID  = GetGameWinApp()->GetUserID();
	ld.nRoomID  = GetGameWinApp()->GetRoomID();
	ld.nTableNO = GetGameWinApp()->GetTableNO();
	ld.nChairNO = GetGameWinApp()->GetChairNO();
	//ld.nActID
	//lstrcpy(ld.szGameCode, APP_NAME);
	lstrcpy(ld.szUserName, GetGameWinApp()->GetUserNameByID(ld.nUserID));
	xyGetHardID(ld.szHardID);
	ld.bIsTrunk = FALSE; //ios用户填TRUE;
	
	CString sRet;
	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(LOTTERY_DRAW);
	
	ProcessRequest(sRet, GR_LOTTERY_DRAW_TOGAME, nDataLen, &ld, nResponse, pRet, FALSE);
	
	return TRUE;
}

BOOL CGameConnect::GC_Task2Game(int nReward)
{
	CGameWinApp* pApp=GetGameWinApp();
	if (!pApp)
		return FALSE;

	TASK_TOGAME	task;
	memset(&task,0,sizeof(task));
	
	task.nUserID  = pApp->GetUserID();
	task.nRoomID  = pApp->GetRoomID();
	task.nTableNO = pApp->GetTableNO();
	task.nChairNO = pApp->GetChairNO();
	lstrcpy(task.szName, pApp->GetUserNameByID(task.nUserID));
	//task.dwIP
	task.nReward = nReward;
		
	CString sRet;
	UINT nResponse;
	LPVOID pRet=NULL;	
	int nDataLen=sizeof(task);
	return ProcessRequest(sRet, GR_TASK_TOGAME, nDataLen, &task, nResponse, pRet, FALSE);
}

BOOL CGameConnect::GC_TakeBonusToGame(CString& sRet, int nBonus/*=0*/)
{
	if (nBonus < 0)
		return FALSE;
	
	TAKE_DEPOSIT_BONUS tdb;
	ZeroMemory(&tdb,sizeof(tdb));
	tdb.nUserID = GetGameWinApp()->GetUserID();
	tdb.nGameID = GetGameWinApp()->GetGameID();
	tdb.nRoomID = GetGameWinApp()->GetRoomID();
	tdb.nTableNO = GetGameWinApp()->GetTableNO();
	tdb.nChairNO = GetGameWinApp()->GetChairNO();
	tdb.nTakeDeposit = nBonus;
	//tdb.dwIPAddr
	xyGetHardID(tdb.szHardID);	
	//tdb.dwFlags
	
	sRet.Empty();
	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(TAKE_DEPOSIT_BONUS);
	
	BOOL bResult = ProcessRequest(sRet, GR_TAKE_BONUS_TO_GAME, nDataLen, &tdb, nResponse, pRet, TRUE);
	
	if (!bResult)
		return FALSE;
	
	if (UR_OPERATE_SUCCEEDED==nResponse)
	{
		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}
	if (UR_OPERATE_FAILED==nResponse)
	{
		sRet=GMS_PROCESS_FAILED;
		SAFE_DELETE_ARRAY(pRet);
		return FALSE;
	}
	if(GR_SAFEBOX_DEPOSIT_MAX==nResponse)
	{
		int nMaxDeposit = *(int*)pRet;
		sRet.Format(GMS_MAX_DEPOSIT, nMaxDeposit);
		SAFE_DELETE_ARRAY(pRet);
		return FALSE;
	}
	if(GR_NODEPOSIT_GAME_EX==nResponse)
	{
		sRet.Format(GMS_NODEPOSIT_GAME,GetGameWinApp()->GetGameName());
		SAFE_DELETE_ARRAY(pRet);
		return FALSE;
	}
	
	GetReturnString(sRet,nResponse);
	
	SAFE_DELETE_ARRAY(pRet);	
	return FALSE;
}

BOOL CGameConnect::GC_LookSafeDeposit(CString& sRet, SAFE_DEPOSIT_EX * pSafeDeposit)
{
	CGameWinApp* pGameApp =  GetGameWinApp();
	if (pGameApp->IsLookOn())
		return FALSE;

	sRet.Empty();
	LOOK_SAFE_DEPOSIT lsd;
	memset(&lsd,0,sizeof(LOOK_SAFE_DEPOSIT));
	lsd.nUserID = pGameApp->GetUserID();
	lsd.nGameID = pGameApp->GetGameID();
	xyGetHardID(lsd.szHardID);

	///////////     Process  Request   ///////////////////////////////////////
	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(LOOK_SAFE_DEPOSIT);
	BOOL bResult = ProcessRequest(sRet,GR_LOOK_SAFE_DEPOSIT, nDataLen, &lsd, nResponse, pRet);

	if(!bResult)//
		return FALSE;

	if(pRet && (nResponse==UR_OPERATE_SUCCEEDED))
	{
		memcpy(pSafeDeposit,pRet,sizeof(SAFE_DEPOSIT_EX));
		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}
	SAFE_DELETE_ARRAY(pRet);
	GetSafeBoxReturnString(sRet,nResponse); 
	return FALSE;
	
}

BOOL CGameConnect::GC_TakeSafeDeposit(CString& sRet,int nTakeDeposit,int nKeyResult)
{
	CGameWinApp* pGameApp = GetGameWinApp();
	if(!pGameApp->IsTakeDepositInGame())
		return FALSE;
	
	sRet.Empty();
	///////////     Process  Request   ///////////////////////////////////////
	TAKE_SAFE_DEPOSIT tsd;
	memset(&tsd,0,sizeof(TAKE_SAFE_DEPOSIT));
	tsd.nUserID = pGameApp->GetUserID();		// 用户ID
	tsd.nGameID = pGameApp->GetGameID();		// 游戏ID
	tsd.nRoomID = pGameApp->GetRoomID();
	tsd.nTableNO = pGameApp->GetTableNO();
	tsd.nChairNO = pGameApp->GetChairNO();
	tsd.nDeposit = nTakeDeposit;				// 银子
	tsd.nKeyResult = nKeyResult;				// 计算结果 = func(保护密码, 随机数)
	//tsd.nPlayingGameID = 0;
	//tsd.nGameVID = 0;
	//tsd.nTransferTotal = 0;					//划入这个游戏的总和(扣除划出的)，注意可能为负值，
	//tsd.nTransferLimit = 0;					//划入这个游戏，限定数量(两,>0)
	//tsd.dwIPAddr = 0;                         //IP地址
	tsd.nGameDeposit = pGameApp->m_GamePlayer[pGameApp->GetChairNO()].nDeposit;    //游戏银子(最终)
	tsd.dwFlags = FLAG_SUPPORT_MONTHLY_LIMIT_EX;
	xyGetHardID(tsd.szHardID);					// 硬件标识符（网卡序列号）
	
	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(TAKE_SAFE_DEPOSIT);
	BOOL bResult = ProcessRequest(sRet,GR_TAKE_SAFE_DEPOSIT, nDataLen, &tsd, nResponse, pRet);
	if(!bResult)//
		return FALSE;

    if(nResponse==UR_OPERATE_SUCCEEDED )///if succeed
	{
		sRet.Format(GMS_MOVESAFEDEPOSIT_SUCCEED);
		return TRUE;
	}
   	if(nResponse==GR_NODEPOSIT_GAME_EX)
	{
		CGameWinApp* pGameApp =  GetGameWinApp();
		sRet.Format(GMS_NODEPOSIT_GAME,pGameApp->GetGameName());
		return FALSE;
	}
	if(nResponse==GR_DEPOSIT_NOTENOUGH_EX)
	{
		sRet.Format(GMS_MOVESAFEDEPOSIT_NOTENOUGH);
		return FALSE;
	}  
	if(nResponse==UR_INVALID_PARAM)
	{
		sRet.Format(GMS_SECUREPWD_KEY_ERROR);
		return FALSE;
	}
	if(nResponse==GR_SAFEBOX_GAME_PLAYING)
	{
		sRet.Format(GMS_PLAYING_GAME_NOTTRANS);
		return FALSE;
	}
	if(nResponse==GR_SAFEBOX_GAME_READY)
	{
		sRet.Format(GMS_GAME_USER_START);
		return FALSE;
	}
	if(nResponse==GR_SAFEBOX_DEPOSIT_MAX)
	{
		int nMaxDeposit = *(int*)pRet;
		sRet.Format(GMS_MAX_DEPOSIT, nMaxDeposit);
		SAFE_DELETE_ARRAY(pRet);
		return FALSE;
	}
	if(nResponse==GR_CONTINUE_PWDWRONG_EX)
	{
		sRet.Format(GMS_CONTINUE_SECUREPWDERROR_TAKEDEPOSIT);
		return FALSE;
	}
	if(nResponse==GR_INPUTLIMIT_DAILY_EX)
	{
		CGameWinApp* pGameApp =  GetGameWinApp();
		int nTransferTotal=*(int*)pRet;
		int nTransferLimit=*(int*)((PBYTE)pRet+sizeof(int));
		int nRemain=nTransferLimit-nTransferTotal;

		if(nRemain>0)
		{
			sRet.Format(GMS_INPUTLIMIT_DAILY,pGameApp->GetGameName(),nTransferLimit ,nRemain);
		}
		else
		{
			sRet.Format(GMS_INPUTLIMIT_TOMORROW,pGameApp->GetGameName(),nTransferLimit);
		}

		SAFE_DELETE_ARRAY(pRet);
		return FALSE;
	}
	else if(nResponse==GR_INPUTLIMIT_MONTHLY_EX)
	{
		LONGLONG llTransfreTotal=*(LONGLONG*)pRet;
		LONGLONG llTransferLimit=*(LONGLONG*)((PBYTE)pRet+sizeof(LONGLONG));
		LONGLONG llRemain=llTransferLimit-llTransfreTotal;
		CString sFormat;
		if(llRemain>0)
		{
			sRet.Format(GMS_INPUTLIMIT_MONTHLY,llTransferLimit,llRemain);
		}
		else
		{
			sRet.Format(GMS_INPUTLIMIT_NEXTMONTH,llTransferLimit);
		}
		
		SAFE_DELETE_ARRAY(pRet);
		return FALSE;
	}
	
    GetSafeBoxReturnString(sRet,nResponse);
	return FALSE;
}

BOOL CGameConnect::GC_SaveDeposit(CString& sRet,int& nSaveDeposit, UINT& nResponse)
{
	CGameWinApp* pGameApp = GetGameWinApp();
	if (!pGameApp->IsTakeDepositInGame())
		return FALSE;
	
	sRet.Empty();
	///////////     Process  Request   ///////////////////////////////////////
	SAVE_SAFE_DEPOSIT ssd;
	memset(&ssd,0,sizeof(SAVE_SAFE_DEPOSIT));
	ssd.nUserID = pGameApp->GetUserID();		// 用户ID
	ssd.nGameID = pGameApp->GetGameID();		// 游戏ID
	ssd.nRoomID = pGameApp->GetRoomID();
	ssd.nTableNO = pGameApp->GetTableNO();
	ssd.nChairNO = pGameApp->GetChairNO();
	ssd.nDeposit = nSaveDeposit;				// 银子
	//ssd.nKeyResult = 0;						// 计算结果 = func(保护密码, 随机数)
	//ssd.nPlayingGameID = 0;
	//ssd.nGameVID = 0;
	//ssd.nTransferTotal = 0;					// 划入这个游戏的总和(扣除划出的)，注意可能为负值，
	//ssd.nTransferLimit = 0;					// 划入这个游戏，限定数量(两,>0)
	//ssd.dwIPAddr = 0;                         // IP地址							
	ssd.nGameDeposit = pGameApp->m_GamePlayer[pGameApp->GetChairNO()].nDeposit;          //游戏银子(最终)
	ssd.dwFlags = FLAG_SUPPORT_KEEPDEPOSIT_EX;
	xyGetHardID(ssd.szHardID);					// 硬件标识符（网卡序列号）
	
	//UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(SAVE_SAFE_DEPOSIT);
	BOOL bResult = ProcessRequest(sRet,GR_SAVE_SAFE_DEPOSIT, nDataLen, &ssd, nResponse, pRet);
	if(!bResult)//
		return FALSE;
	
    if(nResponse==UR_UPDATE_SUCCEEDED )///if succeed
		return TRUE;
   	if(nResponse==GR_NODEPOSIT_GAME_EX)
	{
		CGameWinApp* pGameApp =  GetGameWinApp();
		sRet.Format(GMS_NODEPOSIT_GAME,pGameApp->GetGameName());
		return FALSE;
	}
	if(nResponse==GR_DEPOSIT_NOTENOUGH_EX)
	{
		sRet.Format(GMS_TRANSFER_NOTENOUGH);
		return FALSE;
	}
	if(nResponse==GR_SAFEBOX_GAME_PLAYING)
	{
		sRet.Format(GMS_PLAYING_GAME_NOTTRANS);
		return FALSE;
	}
	if(nResponse==GR_SAFEBOX_GAME_READY)
	{
		sRet.Format(GMS_GAME_USER_START);
		return FALSE;
	}
	if(nResponse==GR_SAFEBOX_DEPOSIT_MIN)
	{
		int nMinDeposit = *(int*)pRet;
		sRet.Format(GMS_MIN_DEPOSIT, nMinDeposit);
		SAFE_DELETE_ARRAY(pRet);
		return FALSE;
	}
	if(nResponse==GR_BOUT_NOTENOUGH_EX)
	{
		int nMinBout=0;
		memcpy(&nMinBout,pRet,sizeof(int));
		SAFE_DELETE_ARRAY(pRet);
		sRet.Format(GMS_OUTPUT_BOUT_NOTENOUGH,nMinBout);
		return FALSE;
	}
	if(nResponse==GR_TIMECOST_NOTENOUGH_EX)
	{
		int nMinMinute=0;
		memcpy(&nMinMinute,pRet,sizeof(int));
		SAFE_DELETE_ARRAY(pRet);
		sRet.Format(GMS_OUTPUT_TIMECOST_NOTENOUGH,nMinMinute);
		return FALSE;
	}

	if (nResponse==GR_KEEPDEPOSIT_LIMIT_EX)
	{ 
		int nGameDeposit=*(int*)pRet;
		int nKeepDeposit=*(int*)((PBYTE)pRet+sizeof(int));
		SAFE_DELETE_ARRAY(pRet);
		sRet.Format(GMS_KEEPDEPOSIT_LIMIT, nGameDeposit-nKeepDeposit, nKeepDeposit);
		nSaveDeposit = nGameDeposit-nKeepDeposit;
		return FALSE;
	}
	
    GetSafeBoxReturnString(sRet,nResponse);
	return FALSE;
}

BOOL CGameConnect::GC_TakeRndKey(CString& sRet,SAFE_RNDKEY * pSafeRndKey,UINT& nResponse)
{
	CGameWinApp* pGameApp =  GetGameWinApp();
	sRet.Empty();
	TAKE_SAFE_RNDKEY tsd;
	memset(&tsd,0,sizeof(TAKE_SAFE_RNDKEY));
	tsd.nUserID = pGameApp->GetUserID();
	tsd.nGameID = pGameApp->GetGameID();
	xyGetHardID(tsd.szHardID);
	///////////     Process  Request   ///////////////////////////////////////
	LPVOID pRet=NULL;
	int nDataLen=sizeof(TAKE_SAFE_RNDKEY);
	BOOL bResult = ProcessRequest(sRet,GR_TAKE_SAFE_RNDKEY, nDataLen, &tsd, nResponse, pRet);
	
	if(!bResult)//
		return FALSE;
	if(pRet && (nResponse==UR_OPERATE_SUCCEEDED))
	{
		memcpy(pSafeRndKey,pRet,sizeof(SAFE_RNDKEY));
		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}
	SAFE_DELETE_ARRAY(pRet);
	
    GetSafeBoxReturnString(sRet,nResponse); //reserved
	return FALSE;	
}


//pengsy
void  CGameConnect::GetSafeBoxReturnString(CString &sRet, UINT nResponse)
{
	switch(nResponse)//if not succeed
	{
	case GR_SERVICE_BUSY_EX:
		sRet=GMS_SERVICE_BUSY;
		break;
	case GR_NO_THIS_FUNCTION_EX:
		sRet=GMS_NO_THIS_FUNCTION;
		break;
	case GR_SYSTEM_LOCKED_EX:
		sRet=GMS_SYSTEM_LOCKED;
		break;
	case GR_TOKENID_MISMATCH_EX:
		sRet=GMS_HARDID_MISMATCHEX;
		break;
	case GR_HARDID_MISMATCH_EX:
		sRet=GMS_HARDID_MISMATCHEX;
		break;
	case GR_PWDLEN_INVALID_EX:
		sRet=GMS_PWDLEN_INVALID;
		break;
	case GR_NEED_LOGON_EX:
		sRet=GMS_NEED_LOGON;
		break;
	case GR_ROOM_NOT_EXIST_EX:
		sRet=GMS_ROOM_NOT_EXIST;
		break;
	case GR_ROOM_CLOSED_EX:
		sRet=GMS_ROOM_CLOSED;
		break;
	case GR_ROOM_NOT_OPENED_EX:
		sRet=GMS_ROOM_NOT_OPENED;
		break;
	case GR_USER_FORBIDDEN_EX:
		sRet=GMS_USER_FORBIDDEN;
		break;
	default:
		sRet=GMS_OPERATE_FAILD;
		break;
	}
}

BOOL CGameConnect::GC_AskNewTableChair(CString& sRet)
{
	CGameWinApp* pGameApp =  GetGameWinApp();
	sRet.Empty();

	if (pGameApp->IsLookOn())
		return FALSE;

	ASK_NEWTABLECHAIR antc;
	memset(&antc,0,sizeof(ASK_NEWTABLECHAIR));

	antc.nUserID = pGameApp->GetUserID();
	antc.nRoomID = pGameApp->GetRoomID();
	antc.nTableNO = pGameApp->GetTableNO();
	antc.nChairNO = pGameApp->GetChairNO();

	///////////     Process  Request   ///////////////////////////////////////
	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(ASK_NEWTABLECHAIR);
	BOOL bResult = ProcessRequest(sRet,GR_ASK_NEW_TABLECHAIR, nDataLen,&antc, nResponse, pRet);

	if (!bResult)
		return FALSE;
	
	if (nResponse == UR_OPERATE_SUCCEEDED)
	{
		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}
	else if (nResponse == GR_RESPONE_ASKNEWTABLE_SPANSHORT)
	{
		sRet=_T(" <z=tc系统通知> <c=255>您的换桌操作过于频繁，请稍等片刻!\r\n");
		GetGameWinApp()->ShowMsgInChatView(sRet);
		SAFE_DELETE_ARRAY(pRet);
		return FALSE;
	}

	SAFE_DELETE_ARRAY(pRet);
    return FALSE;
}

BOOL CGameConnect::GC_AskReRandomTable(CString& sRet)
{
	CGameWinApp* pGameApp =  GetGameWinApp();
	sRet.Empty();
	
	if (!pGameApp 
		|| pGameApp->IsLookOn() 
		|| !pGameApp->IsRandomRoom()
		|| !pGameApp->IsLeaveAlone())
		return FALSE;
	
	ASK_RANDOM_TABLE art;
	memset(&art,0,sizeof(art));	
	art.nUserID = pGameApp->GetUserID();
	art.nRoomID = pGameApp->GetRoomID();
	art.nTableNO = pGameApp->GetTableNO();
	art.nChairNO = pGameApp->GetChairNO();
	
	///////////     Process  Request   ///////////////////////////////////////
	UINT nResponse;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(ASK_RANDOM_TABLE);	
	BOOL bResult = ProcessRequest(sRet,GR_ASK_RANDOM_TABLE, nDataLen, &art, nResponse, pRet);
	
	if (!bResult)
		return FALSE;

	if (nResponse == UR_OPERATE_SUCCEEDED)
	{
		CMainGame* pMainGame=(CMainGame*)pGameApp->GetMainGame();
		if (pMainGame) //请求加入分桌队列成功
		{
			pMainGame->OPE_StopEventKickOff();	
		}
		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}
	else if (nResponse == GR_RESPONE_DEPOSIT_NOTENOUGH)
	{
		GetGameWinApp()->OnDepositNotEnough(pRet,nDataLen);
		SAFE_DELETE_ARRAY(pRet);
		return FALSE;
	}
	else if (nResponse==GR_RESPONE_DEPOSIT_TOOHIGH)
	{
		GetGameWinApp()->OnDepositTooHigh(pRet,nDataLen);
		SAFE_DELETE_ARRAY(pRet);
		return FALSE;
	}

	SAFE_DELETE_ARRAY(pRet);
    return FALSE;
}

void CGameConnect::GetReturnString(CString &sRet, UINT nResponse)
{
	switch(nResponse)//if not succeed
	{
	case UR_REQUEST_UNKNOWN:
		sRet=GMS_REQUEST_UNKNOWN ;
		break;
	case UR_OBJECT_EXIST:
		sRet=GMS_OBJECT_EXIST ;
		break;
	case UR_INVALID_PARAM:
		sRet=GMS_INVALID_PARAM ;
		break;
	case UR_DATABASE_ERROR:
		sRet=GMS_PROCESS_FAILED ;
		break;
	case UR_PASSWORD_WRONG:
		sRet=GMS_PASSWORD_WRONG ;
		break;
	case GR_NAMELEN_INVALID_EX:
		sRet=GMS_NAMELEN_INVALID;
		break;
	case GR_PWDLEN_INVALID_EX:
		sRet=GMS_PWDLEN_INVALID ;
		break;
	case UR_OBJECT_NOT_EXIST:
		sRet=GMS_OBJECT_NOT_EXIST ;
		break;
	case GR_NO_THIS_FUNCTION_EX:
		sRet=GMS_NO_THIS_FUNCTION;
		break;
	case GR_SIMPLE_PASSWORD_EX:
		sRet=GMS_SIMPLE_PASSWORD ;
		break;
	case GR_HARDID_MISMATCH_EX:
		sRet=GMS_HARDID_MISMATCHEX ;
		break;
	case GR_TOKENID_MISMATCH_EX:
		sRet=GMS_CONNECT_DISABLE ;
		break;
	case GR_ROOM_NOT_EXIST_EX:
		sRet=GMS_ROOM_NOT_EXIST ;
		break;
	case GR_ROOM_NOT_OPENED_EX:
		sRet=GMS_ROOM_NOT_OPENED ;
		break;
	case GR_ROOM_CLOSED_EX:
		sRet=GMS_ROOM_CLOSED;
		break;
	case GR_NEED_LOGON_EX:
		sRet=GMS_NEED_LOGON ;
		break;
	case GR_SYSTEM_LOCKED_EX:
		sRet=GMS_SYSTEM_LOCKED ;
		break;
	case GR_USER_FORBIDDEN_EX:
		sRet=GMS_USER_FORBIDDEN ;
		break;
	case GR_NO_PERMISSION_EX:
		sRet=GMS_NO_PERMISSION ;
		break;
	case GR_SERVICE_BUSY_EX:
		sRet=GMS_SERVICE_BUSY ;
		break;
		// 	case GR_MATCH_NOTREADY_EX:
		// 		sRet=HLS_MATCH_NOTREADY;
		// 		break;
		// 	case GR_MATCH_FINISHED_EX:
		// 		sRet=HLS_MATCH_FINISHED;
		// 		break;
	case GR_INVALID_IDCARD_EX:
		sRet=GMS_INVALID_IDCARD;
		break;
	case GR_INVALID_TRUENAME_EX:
		sRet=GMS_INVALID_TRUENAME;
		break;
	default:
		//		sRet=GMS_OPERATE_FAILD;
		break;
	}
}

BOOL CGameConnect::GC_GetWelfareConfig(CString& sRet, int nActivityID, LPGET_WELFARECONFIG_OK pRetData)
{
	CGameWinApp* pGameApp = GetGameWinApp();
	if (!pGameApp)
		return FALSE;
	
	GET_WELFARECONFIG gwfc;
	ZeroMemory(&gwfc, sizeof(gwfc));
	gwfc.nUserID  = pGameApp->GetUserID();
	gwfc.nRoomID  = pGameApp->GetRoomID();
	gwfc.nTableNO = pGameApp->GetTableNO();
	gwfc.nChairNO = pGameApp->GetChairNO();
	gwfc.nActivityID = nActivityID;
	
	sRet.Empty();
	
	///////////     Process  Request   ///////////////////////////////////////
	UINT nResponse=0;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(GET_WELFARECONFIG);
	BOOL bResult = ProcessRequest(sRet, GR_GET_WELFARECONFIG, nDataLen, &gwfc, nResponse, pRet);
	if (!bResult)
		return FALSE;
	
	if (UR_OPERATE_SUCCEEDED == nResponse && pRet)
	{
		memset(pRetData, 0, sizeof(GET_WELFARECONFIG_OK));
		memcpy(pRetData, pRet, sizeof(GET_WELFARECONFIG_OK));
		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}
	
    SAFE_DELETE_ARRAY(pRet);	
   	return FALSE;
}

BOOL CGameConnect::GC_ApplyWelfare(CString& sRet, int nActivityID, int nSaveTo, int& nSendSilver)
{
	CGameWinApp* pGameApp = GetGameWinApp();
	if (!pGameApp || pGameApp->IsLookOn())
		return FALSE;
	
	if (nSaveTo < APPLY_WELFARE_TO_GAME || nSaveTo > APPLY_WELFARE_TO_BACK)
		return FALSE;
	
	APPLY_BASEWELFARE applyWel;
	ZeroMemory(&applyWel, sizeof(applyWel));
	applyWel.nUserID  = pGameApp->GetUserID();
	applyWel.nRoomID  = pGameApp->GetRoomID();
	applyWel.nTableNO = pGameApp->GetTableNO();
	applyWel.nChairNO = pGameApp->GetChairNO();
	sprintf(applyWel.szUserName, "%s", pGameApp->GetUserNameByID(applyWel.nUserID));
	applyWel.nActivityID = nActivityID;
	applyWel.nSaveTo  = nSaveTo;
	
	sRet.Empty();
	nSendSilver=0;
	
	///////////     Process  Request   ///////////////////////////////////////
	UINT nResponse=0;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(APPLY_BASEWELFARE);
	BOOL bResult = ProcessRequest(sRet, GR_APPLY_BASEWELFARE, nDataLen, &applyWel, nResponse, pRet);
	if (!bResult)
		return FALSE;
	
	if (UR_OPERATE_SUCCEEDED == nResponse)
	{
		nSendSilver = *(int*)pRet;
		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}
	
    SAFE_DELETE_ARRAY(pRet);
	return FALSE;
}

BOOL CGameConnect::GC_ApplyWelfareEx(CString& sRet, int nActivityID, int nSaveTo, int& nSendSilver)
{
	CGameWinApp* pGameApp = GetGameWinApp();
	if (!pGameApp || pGameApp->IsLookOn())
		return FALSE;
	
	if (nSaveTo < APPLY_WELFARE_TO_GAME || nSaveTo > APPLY_WELFARE_TO_BACK)
		return FALSE;
	
	APPLY_BASEWELFARE_EX applyWel;
	ZeroMemory(&applyWel, sizeof(applyWel));
	applyWel.nUserID  = pGameApp->GetUserID();
	applyWel.nRoomID  = pGameApp->GetRoomID();
	applyWel.nTableNO = pGameApp->GetTableNO();
	applyWel.nChairNO = pGameApp->GetChairNO();
	sprintf(applyWel.szUserName, "%s", pGameApp->GetUserNameByID(applyWel.nUserID));
	applyWel.nActivityID = nActivityID;
	applyWel.nSaveTo  = nSaveTo;
	xyGetHardID(applyWel.szHardID);
	
	sRet.Empty();
	nSendSilver=0;
	
	///////////     Process  Request   ///////////////////////////////////////
	UINT nResponse=0;
	LPVOID pRet=NULL;
	int nDataLen=sizeof(APPLY_BASEWELFARE_EX);
	BOOL bResult = ProcessRequest(sRet, GR_APPLY_BASEWELFARE_EX, nDataLen, &applyWel, nResponse, pRet);
	if (!bResult)
		return FALSE;
	
	if (UR_OPERATE_SUCCEEDED == nResponse)
	{
		nSendSilver = *(int*)pRet;
		SAFE_DELETE_ARRAY(pRet);
		return TRUE;
	}
	
    SAFE_DELETE_ARRAY(pRet);
	return FALSE;
}