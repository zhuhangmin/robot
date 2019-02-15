#pragma once

#define MAX_DBCONNECT_COUNT		10
#define DEF_DBCONNECT_COUNT		2

#define	DEF_REGISTER_INTERVAL	60 // minutes

class CWorkerContext
{
public:
	CWorkerContext(){ m_bReConnectMain = FALSE; }
public:
	BOOL            m_bReConnectMain;
};

class CSockServer : public CDefIocpServer
{
public:
	CSockServer(const BYTE key[] = 0, const ULONG key_len = 0, DWORD flagEncrypt = 0, DWORD flagCompress = 0) 
		: CDefIocpServer(key, key_len, flagEncrypt, flagCompress){}

	CSockServer(int nKeyType, DWORD flagEncrypt = 0, DWORD flagCompress = 0) 
		: CDefIocpServer(nKeyType, flagEncrypt, flagCompress){}

	BOOL TransmitRequest(LPCONTEXT_HEAD lpContext, LPREQUEST lpRequest, int svrindex);
	BOOL TransmitRequestEx(LPCONTEXT_HEAD lpContext, LPREQUEST lpRequest,int svrindex,void* pData,int nDataLen);

	virtual void* OnWorkerStart();
	virtual void OnWorkerExit(void* pContext);

	virtual BOOL    PutToServer(LPCONTEXT_HEAD lpContext, UINT nRequest, void* pData, int nLen);
	virtual CString GetProfileContent();

protected:
	virtual BOOL OnRequest(void* lpParam1, void* lpParam2);
	virtual BOOL OnUnsupported(LPCONTEXT_HEAD lpContext, LPREQUEST lpRequest, CWorkerContext* pThreadCxt);

	virtual BOOL OnCloseSocket(LPCONTEXT_HEAD lpContext, LPREQUEST lpRequest, CWorkerContext* pThreadCxt);
	virtual BOOL OnConnectSocket(LPCONTEXT_HEAD lpContext, LPREQUEST lpRequest, CWorkerContext* pThreadCxt);

	virtual BOOL SendOpeResponse(LPCONTEXT_HEAD lpContext, BOOL bNeedEcho, REQUEST& response);//有回应的请求
	virtual BOOL SendOpeRequest(LPCONTEXT_HEAD lpContext, REQUEST& response);				  //无回应的请求
	virtual BOOL SendOpeRequest(LPCONTEXT_HEAD lpContext, void* pData, int nLen, REQUEST& response);  //无回应的请求
	virtual BOOL SendOpeReqOnlyCxt(LPCONTEXT_HEAD lpContext, UINT nRepeatHead, void* pData, REQUEST& response); //无回应的请求,只回报文头

	virtual BOOL OnSendPulse(LPCONTEXT_HEAD lpContext, LPREQUEST lpRequest, CWorkerContext* pThreadCxt);
	virtual BOOL OnValidateClient(LPCONTEXT_HEAD lpContext, LPREQUEST lpRequest, CWorkerContext* pThreadCxt);
    virtual BOOL OnGetSvrHealthInfo(LPCONTEXT_HEAD lpContext, LPREQUEST lpRequest, CWorkerContext* pThreadCxt);

private:
    time_t m_tmStartTime;
};

