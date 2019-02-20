#include "stdafx.h"
#include "Robot.h"
#include "Main.h"
#include "RobotReq.h"
#include "common_func.h"
#include "RobotUitls.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

Robot::Robot(const RobotSetting& robot) {
    userid_ = robot.userid;
    password_ = robot.password;
}

Robot::Robot() {

}

Robot::~Robot() {}

void Robot::OnDisconnGame() {
    std::lock_guard<std::mutex> lock(mutex_);
    game_connection_->DestroyEx();
}

int Robot::ConnectGameWithLock(const std::string& strIP, const int32_t nPort, uint32_t nThrdId) {
    game_connection_->InitKey(KEY_GAMESVR_2_0, ENCRYPT_AES, 0);
    //由于目前网络库不支持IPV6，所以添加配置项，可以选定本地地址
    std::string sIp = (g_useLocal) ? "127.0.0.1" : strIP;
    if (!game_connection_->Create(sIp.c_str(), nPort, 5, 0, nThrdId, 0, GetHelloData(), GetHelloLength())) {
        UWL_ERR("ConnectGame Faild! IP:%s Port:%d", sIp.c_str(), nPort);
        //assert(false);
        return kCommFaild;
    }

    ////TODO FIXME DECOUPLE WITH MANAGER
    ////@zhuhangmin 20181129 立即加上token信息 以免丢包
    return kCommFaild;
}

int Robot::SendRequestWithLock(RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool bNeedEcho /*= true*/) {
    int result = RobotUitls::SendRequest(game_connection_, requestid, val, response, bNeedEcho);
    if (result != kCommSucc) {
        UWL_ERR("game send quest fail");
        //assert(false);
    }
    return result;
}

int Robot::SendEnterGame(const ROOM& room, uint32_t nNofifyThrId, std::string sNick, std::string sPortr, int nTableNo, int nChairNo) {
    std::lock_guard<std::mutex> lock(mutex_);
    RequestID nResponse;
    auto pSendData = std::make_unique<BYTE>();
    std::shared_ptr<void> pRetData;
    uint32_t nDataLen = 0;

    //@zhuhangmin 20190218 pb
    TCHAR hard_id[MAX_HARDID_LEN_EX];			// 硬件标识
    xyGetHardID(hard_id);
    game::base::EnterNormalGameReq enter_req;
    enter_req.set_userid(userid_);
    //enter_req.set_roomid(roomid_);  //TODO
    enter_req.set_flag(0);//TODO
    enter_req.set_hardid(hard_id);
    REQUEST response = {};
    auto result = SendRequestWithLock(GR_ENTER_NORMAL_GAME, enter_req, response);
    if (kCommSucc != result) {
        UWL_ERR("ParseFromRequest faild.");
        return kCommFaild;
    }

    game::base::EnterNormalGameResp enter_resp;
    int ret = ParseFromRequest(response, enter_resp);
    if (kCommSucc != ret) {
        UWL_ERR("ParseFromRequest faild.");
        return kCommFaild;
    }

    if (kCommSucc != enter_resp.code()) {
        UWL_ERR("enter game faild. check return[%d]. req = %s", enter_resp.code(), GetStringFromPb(enter_req).c_str());
        return kCommFaild;
    }

    //TODO 银子不够 case

    //@zhuhangmin 20190218 pb
    return kCommSucc;
}

//@zhuhangmin


int Robot::SendGamePulse() {
    /*std::lock_guard<std::mutex> lock(mutex_);
    if (!game_connection_) {
    UWL_ERR("m_ConnGame is nil");
    assert(false);
    return std::make_tuple(false, ERR_CONNECT_NOT_EXIST);
    }


    if (!game_connection_->IsConnected()) {
    UWL_ERR("m_ConnGame not connected");
    assert(false);
    return std::make_tuple(false, ERR_CONNECT_DISABLE);
    }


    GAME_PULSE gp = {};
    gp.nUserID = GetUserID();
    gp.dwAveDelay = 11;
    gp.dwMaxDelay = 22;

    RequestID nResponse;
    std::shared_ptr<void> pRetData;
    uint32_t nDataLen = sizeof(GAME_PULSE);
    return SendRequestWithLock(GR_GAME_PULSE, nDataLen, &gp, nResponse, pRetData, false);*/
    return 0;
}

