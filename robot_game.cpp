#include "stdafx.h"
#include "robot_game.h"
#include "main.h"
#include "common_func.h"
#include "robot_utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

Robot::Robot(UserID userid) {
    userid_ = userid;
}
int Robot::ConnectGame(const std::string& game_ip, const int game_port, const ThreadID game_notify_thread_id) {
    CHECK_GAMEIP(game_ip);
    CHECK_GAMEPORT(game_port);
    std::lock_guard<std::mutex> lock(mutex_);
    game_connection_->InitKey(KEY_GAMESVR_2_0, ENCRYPT_AES, 0);
    if (!game_connection_->Create(game_ip.c_str(), game_port, 5, 0, game_notify_thread_id, 0, GetHelloData(), GetHelloLength())) {
        UWL_ERR("ConnectGame Faild! IP:%s Port:%d", game_ip.c_str(), game_port);
        assert(false);
        return kCommFaild;
    }

    return kCommFaild;
}

int Robot::DisconnectGame() {
    std::lock_guard<std::mutex> lock(mutex_);
    game_connection_->DestroyEx();
    return kCommSucc;
}

BOOL Robot::IsConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return game_connection_->IsConnected();
}

int Robot::SendGameRequestWithLock(const RequestID requestid, const google::protobuf::Message &val, REQUEST& response, const bool need_echo /*= true*/) {
    CHECK_REQUESTID(requestid);
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

    int result = RobotUtils::SendRequestWithLock(game_connection_, requestid, val, response, need_echo);
    if (result != kCommSucc) {
        UWL_ERR("game send quest fail");
        assert(false);
    }
    return result;
}

// 具体业务
int Robot::SendEnterGame(const RoomID roomid) {
    CHECK_ROOMID(roomid);
    std::lock_guard<std::mutex> lock(mutex_);
    TCHAR hard_id[32];
    xyGetHardID(hard_id);

    game::base::EnterNormalGameReq val;
    val.set_userid(userid_);
    val.set_roomid(roomid);
    val.set_flag(kEnterDefault);
    val.set_hardid(hard_id);

    REQUEST response = {};

    auto result = SendGameRequestWithLock(GR_ENTER_NORMAL_GAME, val, response);
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
        return resp.code();
    }

    return kCommSucc;
}

int Robot::SendGamePulse() {
    std::lock_guard<std::mutex> lock(mutex_);
    game::base::PulseReq val;
    val.set_id(userid_);
    REQUEST response = {};
    return SendGameRequestWithLock(GR_GAME_PLUSE, val, response, false);
}

// 属性接口 

TokenID Robot::GetTokenID() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return game_connection_->GetTokenID();
}

// 配置机器人ID 初始化后不在改变,不需要锁保护
UserID Robot::GetUserID() const {
    return userid_;
}


