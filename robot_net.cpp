#include "stdafx.h"
#include "robot_net.h"
#include "common_func.h"
#include "robot_utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

RobotNet::RobotNet(const UserID& userid) :
userid_(userid) {

}

int RobotNet::ConnectGame(const std::string& game_ip, const int& game_port, const ThreadID& game_notify_thread_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_GAMEIP(game_ip);
    CHECK_GAMEPORT(game_port);
    game_ip_ = game_ip;
    game_port_ = game_port;
    game_notify_thread_id_ = game_notify_thread_id;
    if (kCommSucc !=InitDataWithLock()) {
        ASSERT_FALSE_RETURN;
    }
    return kCommFaild;
}

int RobotNet::OnDisconnGame() {
    std::lock_guard<std::mutex> lock(mutex_);
    ResetInitDataWithLock();
    return kCommSucc;
}

BOOL RobotNet::IsConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!game_connection_) {
        LOG_ERROR("m_ConnGame is nil");
        ASSERT_FALSE_RETURN;
    }
    return game_connection_->IsConnected();
}

int RobotNet::SendGameRequestWithLock(const RequestID& requestid, const google::protobuf::Message &val, REQUEST& response, const bool& need_echo /*= true*/) const {
    CHECK_REQUESTID(requestid);
    if (!game_connection_) {
        LOG_ERROR("m_ConnGame is nil");
        ASSERT_FALSE_RETURN;
    }

    if (!game_connection_->IsConnected()) {
        LOG_WARN("m_ConnGame not connected"); // invalid socket handle.
        ASSERT_FALSE_RETURN;
    }

    return RobotUtils::SendRequestWithLock(game_connection_, requestid, val, response, need_echo);
}

int RobotNet::ResetDataWithLock() {
    if (game_connection_) {
        game_connection_->DestroyEx();
    }
    pulse_timeout_count_ = 0;
    return kCommSucc;
}

int RobotNet::InitDataWithLock() {
    game_connection_ = std::make_shared<CDefSocketClient>();
    game_connection_->InitKey(KEY_GAMESVR_2_0, ENCRYPT_AES, 0);
    if (!game_connection_->Create(game_ip_.c_str(), game_port_, 5, 0, game_notify_thread_id_, 0, GetHelloData(), GetHelloLength())) {
        LOG_ERROR("ConnectGame Faild! IP: [%s] Port: [%d]", game_ip_.c_str(), game_port_);
        ASSERT_FALSE_RETURN;
    }

    timestamp_ = time(0);
    return kCommFaild;
}

int RobotNet::ResetInitDataWithLock() {
    // 重置
    if (kCommSucc != ResetDataWithLock()) {
        ASSERT_FALSE_RETURN;
    }

    // 重新初始化
    if (kCommSucc != InitDataWithLock()) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

// 具体业务

// TODO 多个机器人服务器用了同个账号 踢人警告
int RobotNet::SendEnterGame(const RoomID& roomid) {
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

    const auto result = SendGameRequestWithLock(GR_ENTER_NORMAL_GAME, val, response);
    if (result != kCommSucc) {
        LOG_ERROR("game send quest fail");
        ASSERT_FALSE;
        if (result == kOperationFailed) {
            ResetInitDataWithLock();
        }


        return result;
    }

    game::base::EnterNormalGameResp resp;
    const auto ret = ParseFromRequest(response, resp);
    if (kCommSucc != ret) {
        LOG_ERROR("ParseFromRequest faild.");
        return kCommFaild;
    }

    if (kCommSucc != resp.code()) {
        LOG_ERROR("enter game faild. check return[%d]. req =  [%s]", resp.code(), GetStringFromPb(val).c_str());
        return resp.code();
    }

    return kCommSucc;
}

int RobotNet::SendGamePulse() {
    std::lock_guard<std::mutex> lock(mutex_);
    game::base::PulseReq val;
    val.set_id(userid_);
    REQUEST response = {};
    const auto result = SendGameRequestWithLock(GR_GAME_PLUSE, val, response, false);
    if (result != kCommSucc) {
        LOG_ERROR("game send quest fail");
        ASSERT_FALSE;
        if (result == kOperationFailed) {
            ResetInitDataWithLock();
        }
        return result;
    }
    return kCommSucc;
}

// 属性接口 

TokenID RobotNet::GetTokenID() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return game_connection_->GetTokenID();
}

// 配置机器人ID 初始化后不在改变,不需要锁保护
UserID RobotNet::GetUserID() const {
    return userid_;
}

TimeStamp RobotNet::GetTimeStamp() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return timestamp_;
}

void RobotNet::SetTimeStamp(const TimeStamp& val) {
    std::lock_guard<std::mutex> lock(mutex_);
    timestamp_ = val;
}

int RobotNet::SnapShotObjectStatus() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("OBJECT ADDRESS = %x", this);
    LOG_INFO("userid [%d]", userid_);
    LOG_INFO("token [%d]", game_connection_->GetTokenID());

    return kCommSucc;
}
