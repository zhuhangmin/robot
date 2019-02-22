#include "stdafx.h"
#include "Robot.h"
#include "Main.h"
#include "RobotReq.h"
#include "common_func.h"
#include "RobotUitls.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

Robot::Robot(UserID userid) {
    userid_ = userid;
}

Robot::Robot() {

}

int Robot::ConnectGame(const std::string& strIP, const int32_t nPort, uint32_t nThrdId) {
    std::lock_guard<std::mutex> lock(mutex_);
    game_connection_->InitKey(KEY_GAMESVR_2_0, ENCRYPT_AES, 0);
    //由于目前网络库不支持IPV6，所以添加配置项，可以选定本地地址
    std::string sIp = (g_useLocal) ? "127.0.0.1" : strIP;
    if (!game_connection_->Create(sIp.c_str(), nPort, 5, 0, nThrdId, 0, GetHelloData(), GetHelloLength())) {
        UWL_ERR("ConnectGame Faild! IP:%s Port:%d", sIp.c_str(), nPort);
        //assert(false);
        return kCommFaild;
    }

    return kCommFaild;
}

void Robot::DisconnectGame() {
    std::lock_guard<std::mutex> lock(mutex_);
    game_connection_->DestroyEx();
}

void Robot::IsConnected() {
    std::lock_guard<std::mutex> lock(mutex_);
    game_connection_->IsConnected();
}

int Robot::SendGameRequest(RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool bNeedEcho /*= true*/) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!game_connection_) {
        UWL_ERR("m_ConnGame is nil");
        assert(false);
        return kCommFaild;
    }


    if (!game_connection_->IsConnected()) {
        UWL_WRN("m_ConnGame not connected"); // invalid socket handle.
        //assert(false);
        return kCommFaild;
    }

    int result = RobotUitls::SendRequest(game_connection_, requestid, val, response, bNeedEcho);
    if (result != kCommSucc) {
        UWL_ERR("game send quest fail");
        assert(false);
    }
    return result;
}

// 具体业务
int Robot::SendEnterGame(RoomID roomid, uint32_t nNofifyThrId, std::string sNick, std::string sPortr, int nTableNo, int nChairNo) {

    TCHAR hard_id[MAX_HARDID_LEN_EX];
    xyGetHardID(hard_id);

    game::base::EnterNormalGameReq val;
    val.set_userid(userid_);
    val.set_roomid(roomid);
    val.set_flag(kEnterDefault);
    val.set_hardid(hard_id);

    REQUEST response = {};

    auto result = SendGameRequest(GR_ENTER_NORMAL_GAME, val, response);
    if (kCommSucc != result) {
        UWL_ERR("ParseFromRequest faild.");
        return kCommFaild;
    }

    game::base::EnterNormalGameResp resp;
    int ret = ParseFromRequest(response, resp);
    if (kCommSucc != ret) {
        UWL_ERR("ParseFromRequest faild.");
        return kCommFaild;
    }

    if (kCommSucc != resp.code()) {
        UWL_ERR("enter game faild. check return[%d]. req = %s", resp.code(), GetStringFromPb(val).c_str());
        return kCommFaild;
    }

    //TODO 银子不够 case

    return kCommSucc;
}

void Robot::SendGamePulse() {
    game::base::PulseReq val;
    val.set_id(userid_);
    REQUEST response = {};
    SendGameRequest(GR_GAME_PLUSE, val, response, false);
}

// 属性接口
int32_t Robot::GetUserID() {
    std::lock_guard<std::mutex> lock(mutex_);
    return userid_;
}

TokenID Robot::GetTokenID() {
    std::lock_guard<std::mutex> lock(mutex_);
    return game_connection_->GetTokenID();
}
