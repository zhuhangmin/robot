#include "stdafx.h"
#include "robot_net.h"
#include "main.h"
#include "common_func.h"
#include "robot_utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

RobotNet::RobotNet(UserID userid) {
    userid_ = userid;
}
int RobotNet::ConnectGame(const std::string& game_ip, const int game_port, const ThreadID game_notify_thread_id) {
    CHECK_GAMEIP(game_ip);
    CHECK_GAMEPORT(game_port);
    std::lock_guard<std::mutex> lock(mutex_);
    game_connection_ = std::make_shared<CDefSocketClient>();
    game_connection_->InitKey(KEY_GAMESVR_2_0, ENCRYPT_AES, 0);
    if (!game_connection_->Create(game_ip.c_str(), game_port, 5, 0, game_notify_thread_id, 0, GetHelloData(), GetHelloLength())) {
        LOG_ERROR("ConnectGame Faild! IP:%s Port:%d", game_ip.c_str(), game_port);
        ASSERT_FALSE_RETURN;
    }

    return kCommFaild;
}

int RobotNet::OnDisconnGame() {
    std::lock_guard<std::mutex> lock(mutex_);
    game_connection_->DestroyEx();
    return kCommSucc;
}

BOOL RobotNet::IsConnected() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!game_connection_) {
        LOG_ERROR("m_ConnGame is nil");
        ASSERT_FALSE_RETURN;
    }
    return game_connection_->IsConnected();
}

int RobotNet::SendGameRequestWithLock(const RequestID requestid, const google::protobuf::Message &val, REQUEST& response, const bool need_echo /*= true*/) {
    CHECK_REQUESTID(requestid);
    if (!game_connection_) {
        LOG_ERROR("m_ConnGame is nil");
        ASSERT_FALSE_RETURN;
    }

    if (!game_connection_->IsConnected()) {
        UWL_WRN("m_ConnGame not connected"); // invalid socket handle.
        //ASSERT_FALSE;
        return kCommFaild;
    }

    int result = RobotUtils::SendRequestWithLock(game_connection_, requestid, val, response, need_echo);
    if (result != kCommSucc) {
        LOG_ERROR("game send quest fail");
        if (result == RobotErrorCode::kOperationFailed) {

        }

        ASSERT_FALSE_RETURN;
    }
    return result;
}

// 具体业务

// TODO 多个机器人服务器用了同个账号 踢人警告
int RobotNet::SendEnterGame(const RoomID roomid) {
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
        LOG_ERROR("ParseFromRequest faild.");
        return kCommFaild;
    }

    game::base::EnterNormalGameResp resp;
    int ret = ParseFromRequest(response, resp);
    if (kCommSucc != ret) {
        LOG_ERROR("ParseFromRequest faild.");
        return kCommFaild;
    }

    if (kCommSucc != resp.code()) {
        LOG_ERROR("enter game faild. check return[%d]. req = %s", resp.code(), GetStringFromPb(val).c_str());
        return resp.code();
    }

    return kCommSucc;
}

int RobotNet::SendGamePulse() {
    std::lock_guard<std::mutex> lock(mutex_);
    game::base::PulseReq val;
    val.set_id(userid_);
    REQUEST response = {};
    return SendGameRequestWithLock(GR_GAME_PLUSE, val, response, false);
}

// 属性接口 

TokenID RobotNet::GetTokenID() {
    std::lock_guard<std::mutex> lock(mutex_);
    return game_connection_->GetTokenID();
}

// 配置机器人ID 初始化后不在改变,不需要锁保护
UserID RobotNet::GetUserID() const {
    return userid_;
}

int RobotNet::SnapShotObjectStatus() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("OBJECT ADDRESS = %x", this);
    LOG_INFO("userid [%d]", userid_);
    LOG_INFO("token [%d]", game_connection_->GetTokenID());

    return kCommSucc;
}
