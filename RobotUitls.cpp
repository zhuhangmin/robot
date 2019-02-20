#include "stdafx.h"
#include "RobotUitls.h"

int RobotUitls::SendRequest(CDefSocketClientPtr& connection_, RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool bNeedEcho /*= true*/) {
    CONTEXT_HEAD	context_head = {};
    context_head.hSocket = connection_->GetSocket();
    context_head.lSession = 0;
    context_head.bNeedEcho = bNeedEcho;

    std::unique_ptr<char> data(new char[val.ByteSize()]);
    bool is_succ = val.SerializePartialToArray(data.get(), val.ByteSize());
    if (false == is_succ) {
        UWL_ERR("SerializePartialToArray failed.");
        return kCommFaild;
    }

    REQUEST request(requestid, data.get(), val.ByteSize());

    BOOL timeout = false;
    BOOL result = connection_->SendRequest(&context_head, &request, &response, timeout, REQ_TIMEOUT_INTERVAL);

    if (!result)///if timeout or disconnect 
    {
        UWL_ERR("send request fail");
        //assert(false);
        return timeout ? ERROR_CODE::REQUEST_TIMEOUT : ERROR_CODE::OPERATION_FAILED;
    }

    auto nRespId = response.head.nRequest;

    if (0 == nRespId) {
        assert(false);
        /*       UWL_ERR("game respId = 0");
        CHAR info[512] = {};
        sprintf_s(info, "%s", pRetData.get());*/
        return kCommFaild;
    }
    return kCommSucc;
}
CString ExecHttpRequestPost(const CString& strUrl, const CString& strParams) {
    CString		strResult, strServer, strObject;
    CInternetSession* pSession = NULL;
    CHttpConnection*   pHttpConn = NULL;
    CHttpFile*   pHTTPFile = NULL;
    INTERNET_PORT   nPort;
    DWORD   dwServiceType;
    DWORD  retcode = -1;
    try {
        pSession = new CInternetSession();
        (void) pSession->SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, HTTP_CONNECT_TIMEOUT);	//重试之间的等待延时
        (void) pSession->SetOption(INTERNET_OPTION_CONNECT_RETRIES, 1);			//重试次数

        (void) AfxParseURL((LPCTSTR) strUrl, dwServiceType, strServer, strObject, nPort);
        pHttpConn = pSession->GetHttpConnection(strServer, nPort);
        pHTTPFile = pHttpConn->OpenRequest(0, strObject, NULL, 1, NULL, "HTTP/1.1", INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_AUTO_REDIRECT);
        if (pHTTPFile) {
            (void) pHTTPFile->AddRequestHeaders("Content-Type:   application/json");
            (void) pHTTPFile->AddRequestHeaders("Accept:   */*");
            (void) pHTTPFile->SendRequest(NULL, 0, (LPTSTR) (LPCTSTR) strParams, strParams.GetLength());
            (void) pHTTPFile->QueryInfoStatusCode(retcode);

            CString   text;
            while (pHTTPFile->ReadString(text)) {
                strResult += text;
            }
        }
    } catch (...) {
        UwlTrace(_T("ExecHttpRequestPost catch:%s retcode:%d error: %d"), strUrl, retcode, GetLastError());
        UwlLogFile(_T("ExecHttpRequestPost catch:%s retcode:%d error: %d"), strUrl, retcode, GetLastError());
        assert(false);
    };

    if (pHTTPFile) { pHTTPFile->Close(); delete  pHTTPFile;  pHTTPFile = NULL; }
    if (pHttpConn) { pHttpConn->Close(); delete  pHttpConn;  pHttpConn = NULL; }
    if (pSession) { pSession->Close();  delete   pSession;   pSession = NULL; }

    if (strResult == "") {
        UwlTrace(_T("ExecHttpRequestPost urlPath:%s retcode:%d error: %d"), strUrl, retcode, GetLastError());
        UwlLogFile(_T("ExecHttpRequestPost urlPath:%s retcode:%d error: %d"), strUrl, retcode, GetLastError());
        assert(false);

    } else {
        UwlLogFile(_T("ExecHttpRequestPost strResult:%s "), strResult);
    }
    return strResult;
}
