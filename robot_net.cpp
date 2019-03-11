#include "stdafx.h"
#include "robot_net.h"
#include "common_func.h"
#include "robot_utils.h"
#include "setting_manager.h"
#include "robot_statistic.h"

RobotNet::RobotNet(const UserID& userid) :
userid_(userid) {

}

int RobotNet::Connect(const std::string& game_ip, const int& game_port, const ThreadID& game_notify_thread_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (connection_) {
        if (connection_->IsConnected()) {
            return kCommSucc;
        }
    }
    CHECK_GAMEIP(game_ip);
    CHECK_GAMEPORT(game_port);
    game_ip_ = game_ip;
    game_port_ = game_port;
    game_notify_thread_id_ = game_notify_thread_id;
    if (kCommSucc !=InitDataWithLock()) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

BOOL RobotNet::IsConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_CONNECTION(connection_);
    return connection_->IsConnected();
}

int RobotNet::SendGameRequestWithLock(const RequestID& requestid, const google::protobuf::Message &val, REQUEST& response, const bool& need_echo /*= true*/) const {
    CHECK_REQUESTID(requestid);
    CHECK_CONNECTION(connection_);

    if (!connection_) {
        LOG_ERROR("connection not exist");
        return kCommFaild;
    }

    if (!connection_->IsConnected()) {
        LOG_ERROR("not connected");
        return kCommFaild;
    }

    return RobotUtils::SendRequestWithLock(connection_, requestid, val, response, need_echo);
}

int RobotNet::OnDisconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_WARN("OnDisconnect Game Robot");
    ResetDataWithLock();
    return kCommSucc;
}

int RobotNet::ResetDataWithLock() {
    CHECK_CONNECTION(connection_);
    connection_->DestroyEx();
    timeout_count_ = 0;
    need_reconnect_ = true;
    return kCommFaild;
}

int RobotNet::InitDataWithLock() {
    connection_ = std::make_shared<CDefSocketClient>();
    connection_->InitKey(KEY_GAMESVR_2_0, ENCRYPT_AES, 0);
    if (!connection_->Create(game_ip_.c_str(), game_port_, 5, 0, game_notify_thread_id_, 0, GetHelloData(), GetHelloLength())) {
        LOG_ERROR("ConnectGame Faild! IP: [%s] Port: [%d]", game_ip_.c_str(), game_port_);
        EVENT_TRACK(EventType::kErr, kCreateGameConnFailed);
        ASSERT_FALSE_RETURN;
    }

    timestamp_ = time(0);
    need_reconnect_ = false;
    return kCommSucc;
}

// 具体业务

// TODO 多个机器人服务器用了同个账号 踢人警告
int RobotNet::SendEnterGame(const RoomID& roomid, const TableNO& tableno) {
    CHECK_ROOMID(roomid);
    std::lock_guard<std::mutex> lock(mutex_);
    TCHAR hard_id[32];
    xyGetHardID(hard_id);

    game::base::EnterNormalGameReq val;
    val.set_userid(userid_);
    val.set_gameid(SettingMgr.GetGameID());
    val.set_target(tableno);// 
    val.set_roomid(roomid);
    val.set_flag(kEnterWithTargetTable);
    val.set_hardid(hard_id);

    REQUEST response = {};
    RequestID requestid = GR_ENTER_NORMAL_GAME;
    const auto result = SendGameRequestWithLock(requestid, val, response);
    if (kCommSucc != result) {
        LOG_ERROR("userid [%d] roomid [%d] requestid [%d] [%s] content [%s] failed, result [%d]", userid_, roomid, requestid, REQ_STR(requestid), GetStringFromPb(val).c_str(), result);
        ASSERT_FALSE;
        if (result == kOperationFailed) {
            ResetDataWithLock();
        }

        if (result == kInvalidUser) { // 用户不合法（token和entergame时保存的token不一致）
            LOG_ERROR("token dismatch with enter game token");
        }

        if (result == kInvalidRoomID) {

        }

        if (result == kAllocTableFaild) {

        }

        if (result == kUserNotFound) {// 用户不存在 userid <= 0

        }

        if (result == kTableNotFound) {

        }

        if (result == kHall_InvalidHardID) {

        }

        if (result == kHall_InOtherGame) {

        }

        if (result == kHall_InvalidRoomID) {

        }
        return result;
    }

    game::base::EnterNormalGameResp resp;
    const auto ret = ParseFromRequest(response, resp);
    if (kCommSucc != ret) {
        LOG_ERROR("userid [%d] roomid [%d] requestid [%d] [%s] content [%s] failed, result [%d]", userid_, roomid, requestid, REQ_STR(requestid), GetStringFromPb(val).c_str(), result);
        ASSERT_FALSE_RETURN;
    }

    auto code = resp.code();
    if (kCommSucc != code) {
        LOG_ERROR("userid [%d] roomid [%d] requestid [%d] [%s] content [%s] failed, resp error code [%d] [%s]", userid_, roomid, requestid, REQ_STR(requestid), GetStringFromPb(val).c_str(), code, ERR_STR(code));
        ASSERT_FALSE;
        return code;
    }

    return kCommSucc;
}

int RobotNet::SendGamePulse() {
    std::lock_guard<std::mutex> lock(mutex_);
    game::base::PulseReq val;
    val.set_id(userid_);
    REQUEST response = {};
    // TODO    GR_RS_PULSE 
    const auto result = SendGameRequestWithLock(GR_GAME_PLUSE, val, response); // 游戏心跳需要回包
    if (kCommSucc != result) {
        LOG_ERROR("game send quest fail");
        ASSERT_FALSE;
        if (result == kOperationFailed) {
            ResetDataWithLock();
        }
        return result;
    }
    return kCommSucc;
}


int RobotNet::KeepConnection() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (need_reconnect_) {
        return ResetDataWithLock();
    }
    return kCommSucc;
}

// 属性接口 

int RobotNet::GetTokenID(TokenID& token) const {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_CONNECTION(connection_);
    token = connection_->GetTokenID();
    return kCommSucc;
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
    CHECK_CONNECTION(connection_);
    LOG_INFO("OBJECT ADDRESS = %x", this);
    LOG_INFO("connection = %x", connection_.get());
    LOG_INFO("game ip [%s]", game_ip_.c_str());
    LOG_INFO("game port [%d]", game_port_);
    LOG_INFO("userid [%d]", userid_);
    LOG_INFO("token [%d]", connection_->GetTokenID());

    return kCommSucc;
}
