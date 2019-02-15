#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


void* CSockServer::OnWorkerStart()
{
	HRESULT hr = ::CoInitialize(NULL);
	if(FAILED(hr)) return NULL;
	
	CWorkerContext* pThreadCxt = new CWorkerContext();
	
    m_tmStartTime = time(NULL);
	return pThreadCxt;
}

void CSockServer::OnWorkerExit(void* pContext)
{
	CWorkerContext* pThreadCxt = (CWorkerContext*)(pContext);
	if(pThreadCxt)
	{
	}
	
	SAFE_DELETE(pThreadCxt);
	::CoUninitialize();
}


BOOL CSockServer::OnRequest(void* lpParam1, void* lpParam2)
{try{
	LPCONTEXT_HEAD	lpContext = LPCONTEXT_HEAD	(lpParam1);
	LPREQUEST		lpRequest = LPREQUEST		(lpParam2);
	
	UwlTrace(_T("----------------------start of request process-------------------"));
	
#if defined(_UWL_TRACE) | defined(UWL_TRACE)
	DWORD dwTimeStart = GetTickCount();
#else
	DWORD dwTimeStart = 0;
#endif
	CWorkerContext* pThreadCxt = (CWorkerContext*)(GetWorkerContext());

	assert(lpContext && lpRequest);
	UwlTrace(_T("socket = %ld requesting..."), lpContext->hSocket);
 
	switch(lpRequest->head.nRequest){
	case UR_SOCKET_CONNECT:
		UwlTrace(_T("UR_SOCKET_CONNECT requesting..."));
		OnConnectSocket(lpContext, lpRequest, pThreadCxt);
		break;
	case UR_SOCKET_CLOSE:
		UwlTrace(_T("UR_SOCKET_CLOSE requesting..."));
		OnCloseSocket(lpContext, lpRequest, pThreadCxt);
		break;
	case UR_SOCKET_ERROR:
		UwlTrace(_T("UR_SOCKET_ERROR requesting..."));
		//OnCloseSocket(lpContext, lpRequest, pThreadCxt);
		break;
	case GR_SEND_PULSE_EX:
		UwlTrace(_T("GR_SEND_PULSE_EX requesting..."));
		OnSendPulse(lpContext, lpRequest, pThreadCxt);
		break;
	case GR_VALIDATE_CLIENT:
		OnValidateClient(lpContext, lpRequest, pThreadCxt);
		break;
    case GR_GET_SVR_HEALTH_INFO:
        OnGetSvrHealthInfo(lpContext, lpRequest, pThreadCxt);
        break;
	default:
		UwlTrace(_T("unsupport requesting..."));
		OnUnsupported(lpContext, lpRequest, pThreadCxt);
		break;
	}
	UwlClearRequest(lpRequest);

#if defined(_UWL_TRACE) | defined(UWL_TRACE)
	DWORD dwTimeEnd = GetTickCount();
#else
	DWORD dwTimeEnd = 0;
#endif
	UwlTrace(_T("request process time costs: %d ms"), dwTimeEnd - dwTimeStart);
	UwlTrace(_T("----------------------end of request process---------------------\r\n"));
}
catch(...)
{
	LPREQUEST		lpReq  = LPREQUEST(lpParam2);
   	UwlLogFile("catch error!!! reqid=%ld,subreqid=%ld,DataLen=%ld",lpReq->head.nRequest,lpReq->head.nSubReq,lpReq->nDataLen );

}
	return TRUE;
}

BOOL CSockServer::OnUnsupported(LPCONTEXT_HEAD lpContext, LPREQUEST lpRequest,
							CWorkerContext* pThreadCxt)
{
	UwlTrace(_T("unsupported request from socket = %d"), lpContext->hSocket);

	return TRUE;
}

BOOL CSockServer::PutToServer(LPCONTEXT_HEAD lpContext, UINT nRequest, void* pData, int nLen)
{
	LPREQUEST pRequest = new REQUEST;
	ZeroMemory(pRequest, sizeof(REQUEST));

	pRequest->head.nRequest = nRequest;
	pRequest->pDataPtr = pData;
	pRequest->nDataLen = nLen;

	LPCONTEXT_HEAD pContext = new CONTEXT_HEAD;
	ZeroMemory(pContext, sizeof(CONTEXT_HEAD));
	
	memcpy(pContext, lpContext, sizeof(CONTEXT_HEAD));

	return PutRequestToWorker(pRequest->nDataLen, DWORD(pContext->hSocket), 
						pContext, pRequest, pRequest->pDataPtr);
}

BOOL CSockServer::SendOpeResponse(LPCONTEXT_HEAD lpContext, BOOL bNeedEcho, REQUEST& response)
{
	BOOL bSendOK = FALSE;
	CONTEXT_HEAD context;
	memcpy(&context, lpContext, sizeof(context));
	context.bNeedEcho = bNeedEcho;
	bSendOK = SendResponse(lpContext->hSocket, &context, &response);
	return bSendOK;
}

BOOL CSockServer::SendOpeRequest(LPCONTEXT_HEAD lpContext, REQUEST& response)
{
	CONTEXT_HEAD context;
	memset(&context,0,sizeof(context));
	memcpy(&context, lpContext, sizeof(context));
	context.bNeedEcho = FALSE; 
	
	BOOL bSendOK=SendRequest(lpContext->hSocket, &context, &response);
	return bSendOK;
}

BOOL CSockServer::SendOpeRequest(LPCONTEXT_HEAD lpContext, void* pData, int nLen, REQUEST& response)
{
	CONTEXT_HEAD context;
	memset(&context,0,sizeof(context));
	memcpy(&context, lpContext, sizeof(context));
	context.bNeedEcho = FALSE; 
	
	PBYTE pNewData = NULL;
	pNewData = new BYTE[nLen];
	memset(pNewData,0,nLen);
	memcpy(pNewData, pData, nLen);
	response.pDataPtr = pNewData;//!
	response.nDataLen = nLen;
	
	BOOL bSendOK=SendRequest(lpContext->hSocket, &context, &response);
	UwlClearRequest(&response);
	return bSendOK;
}

BOOL CSockServer::SendOpeReqOnlyCxt(LPCONTEXT_HEAD lpContext, UINT nRepeatHead, void* pData, REQUEST& response)
{
	if (0==nRepeatHead || !pData)
		return FALSE;

	int nLen = nRepeatHead*sizeof(CONTEXT_HEAD);
	return SendOpeRequest(lpContext, pData, nLen, response);
}

CString CSockServer::GetProfileContent() 
{
	CString  sRet;
	HANDLE hFile=CreateFile( g_szIniFile, 
		GENERIC_WRITE|GENERIC_READ,
		FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
		OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(hFile!=INVALID_HANDLE_VALUE)
    {
		DWORD dwRead;
		DWORD dwFileSize=GetFileSize(hFile,NULL);
		SetFilePointer(hFile,0,NULL,FILE_BEGIN);
		ReadFile(hFile,(LPVOID)sRet.GetBuffer(dwFileSize),dwFileSize,&dwRead,NULL);
		sRet.ReleaseBuffer();
		CloseHandle(hFile);
	}
	return sRet;
}

BOOL CSockServer::OnConnectSocket(LPCONTEXT_HEAD lpContext, LPREQUEST lpRequest, CWorkerContext* pThreadCxt)
{
	CAutoLock lock(&g_csTokenSock);
	g_mapTokenSock[lpContext->lTokenID] = lpContext->hSocket;

	try{
		UWL_ADDR uwlIP = GetClientAddress(lpContext->hSocket, lpContext->lTokenID);
		TCHAR szIP[MAX_SERVERIP_LEN];
		ZeroMemory(szIP, sizeof(szIP));
		UwlAddrToName(uwlIP, szIP);
		UwlLogFile(_T("client connected. ip: %s hSocket: %d"), szIP, lpContext->hSocket);
	}catch(...){
		UwlLogFile(_T("try to get connected ip failed!"));
	}
	return TRUE;
}

BOOL CSockServer::OnCloseSocket(LPCONTEXT_HEAD lpContext, LPREQUEST lpRequest, CWorkerContext* pThreadCxt)
{
	CAutoLock lock(&g_csTokenSock);
	auto it = g_mapTokenSock.find(lpContext->lTokenID);
	if (it != g_mapTokenSock.end() && it->second == lpContext->hSocket){
		g_mapTokenSock.erase(lpContext->lTokenID);
	}	
	return TRUE;
}

BOOL CSockServer::OnSendPulse(LPCONTEXT_HEAD lpContext, LPREQUEST lpRequest, CWorkerContext* pThreadCxt)
{
	return TRUE;
}

BOOL CSockServer::OnValidateClient(LPCONTEXT_HEAD lpContext, LPREQUEST lpRequest, CWorkerContext* pThreadCxt)
{
	int nRepeated = lpRequest->head.nRepeated;
	
	LPVALIDATE_CLIENT lpValidateClient = LPVALIDATE_CLIENT(PBYTE(lpRequest->pDataPtr) 
									+ nRepeated * sizeof(CONTEXT_HEAD));
	if (CLIENT_TYPE_HALL == lpValidateClient->nClientType)
	{
	}
	return TRUE;
}

BOOL CSockServer::OnGetSvrHealthInfo(LPCONTEXT_HEAD lpContext, LPREQUEST lpRequest, CWorkerContext* pThreadCxt)
{
    GET_SVRHEALTH_INFO_OK stInfoOk = { 0 };

    REQUEST response;
    memset(&response, 0, sizeof(response));

    stInfoOk.nClientCount = m_lClientCount;
    stInfoOk.nListNum = GetListCount();
    stInfoOk.nTotalIn = GetTotalPutinListCount();
    stInfoOk.nTotalOut = GetTotalTakeoutListCount();
    stInfoOk.nStartTime = (int)m_tmStartTime;

    response.head.nRequest = UR_FETCH_SUCCEEDED;
    response.pDataPtr = &stInfoOk;
    response.nDataLen = sizeof(stInfoOk);

    BOOL bSendOK = FALSE;

    CONTEXT_HEAD context;
    memcpy(&context, lpContext, sizeof(context));
    context.bNeedEcho = FALSE;

    bSendOK = SendResponse(lpContext->hSocket, &context, &response);

    return TRUE;
}