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
    bool result = connection_->SendRequest(&context_head, &request, &response, timeout, REQ_TIMEOUT_INTERVAL);

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
