#pragma once
#include "RobotDef.h"
class RobotUitls {
public:
    static int SendRequest(CDefSocketClientPtr& connection_, RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool bNeedEcho = true);

    static CString ExecHttpRequestPost(const CString& strUrl, const CString& strParams);

    static int GenRandInRange(int min_value, int max_value, int& res);
};

