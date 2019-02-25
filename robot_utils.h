#pragma once
#include "robot_define.h"
class RobotUitls {
public:
    static int SendRequestWithLock(CDefSocketClientPtr& connection, RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool need_echo = true);

    static CString ExecHttpRequestPost(const CString& url, const CString& params);

    static int GenRandInRange(int min_value, int max_value, int& random_result);
};

