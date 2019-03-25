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

int RobotNet::IsConnected(BOOL& is_connected) const {
    if (!connection_) {
        is_connected = false;
        return kCommSucc;
    }
    is_connected = connection_->IsConnected();
    return kCommSucc;
}

int RobotNet::SendGameRequestWithLock(const RequestID& requestid, const google::protobuf::Message &val, REQUEST& response, const bool& need_echo /*= true*/) const {
    CHECK_REQUESTID(requestid);
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
    LOG_WARN("OnDisconnect Game Robot");
    ResetDataWithLock();
    return kCommSucc;
}

int RobotNet::ResetDataWithLock() {
    if (connection_) {
        LOG_WARN("token [%d] [%s] DestroyEx", connection_->GetTokenID(), __FUNCTION__);
        connection_->DestroyEx();
    }
    timeout_count_ = 0;
    need_reconnect_ = true;
    return kCommFaild;
}

int RobotNet::InitDataWithLock() {
    connection_ = std::make_shared<CDefSocketClient>();
    LOG_INFO("\t[START] userid [%d] robot try connect game ... ip: [%s] port: [%d]", userid_, game_ip_.c_str(), game_port_);
    connection_->InitKey(KEY_GAMESVR_2_0, ENCRYPT_AES, 0);
    if (!connection_->Create(game_ip_.c_str(), game_port_, 5, 0, game_notify_thread_id_, 0, GetHelloData(), GetHelloLength())) {
        LOG_ERROR("\t[START] userid [%d] robot connect game failed! ip: [%s] port: [%d]", userid_, game_ip_.c_str(), game_port_);
        EVENT_TRACK(EventType::kErr, kCreateGameConnFailed);
        return kCommFaild;
    }

    timestamp_ = std::time(nullptr);
    need_reconnect_ = false;
    LOG_INFO("\t[START] userid [%d] robot token [%d] connect game ok! ip: [%s] port: [%d]",
             userid_, connection_->GetTokenID(), game_ip_.c_str(), game_port_);
    return kCommSucc;
}

// 具体业务
int RobotNet::SendEnterGame(const RoomID& roomid, const TableNO& tableno) {
    CHECK_ROOMID(roomid);
    TCHAR hard_id[32];
    xyGetHardID(hard_id);
    game::base::EnterNormalGameReq val;
    val.set_userid(userid_);
    val.set_gameid(SettingConfig.GetGameID());
    val.set_target(tableno);// 
    val.set_roomid(roomid);
    val.set_flag(kEnterWithTargetTable);
    val.set_hardid(hard_id);

    REQUEST response = {};
    const RequestID requestid = GR_ENTER_NORMAL_GAME;
    const auto result = SendGameRequestWithLock(requestid, val, response);
    if (kCommSucc != result) {
        LOG_ERROR("userid [%d] roomid [%d] requestid [%d] [%s] content [%s] failed, result [%d]", userid_, roomid, requestid, REQ_STR(requestid), GetStringFromPb(val).c_str(), result, ERR_STR(result));
        ASSERT_FALSE;
        if (result == kOperationFailed) {
            ResetDataWithLock();
        }
        return result;
    }

    game::base::EnterNormalGameResp resp;
    const auto ret = ParseFromRequest(response, resp);
    if (kCommSucc != ret) {
        LOG_ERROR("userid [%d] roomid [%d] requestid [%d] [%s] content [%s] failed, result [%d]", userid_, roomid, requestid, REQ_STR(requestid), GetStringFromPb(val).c_str(), result);
        ASSERT_FALSE_RETURN;
    }

    const auto code = resp.code();
    if (kCommSucc != code) {
        LOG_ERROR("userid [%d] roomid [%d] requestid [%d] [%s] content [%s] failed, resp error code [%d] [%s]", userid_, roomid, requestid, REQ_STR(requestid), GetStringFromPb(val).c_str(), code, ERR_STR(code));
        if (kHall_InOtherGame == code) {
            LOG_WARN("userid [%d] kHall_InOtherGame, other game id [%d]", userid_, resp.gameid());
        }

        if (kHall_UserNotLogon == code) {
            LOG_WARN("userid[%d] kHall_UserNotLogon, need hall logon", userid_);
        }

        if (kNoFreeChair == code) {
            LOG_WARN("userid[%d] kNoFreeChair, no free chair", userid_);
        }

        if (kTooLessDeposit == code || kTooMuchDeposit == code) {
            LOG_ERROR("[DEPOSIT] userid [%d] roomid [%d] requestid [%d] [%s] content [%s] failed, resp error code [%d] [%s]", userid_, roomid, requestid, REQ_STR(requestid), GetStringFromPb(val).c_str(), code, ERR_STR(code));
        }

        if (kAllocTableFaild == code) {
        }

        if (kHall_InvalidHardID == code) {
        }
        return code;
    }

    return kCommSucc;
}

int RobotNet::KeepConnection() {
    if (need_reconnect_) {
        return InitDataWithLock();
    }
    return kCommSucc;
}

// 属性接口 
int RobotNet::GetTokenID(TokenID& token) const {
    if (!connection_) return kCommFaild;
    token = connection_->GetTokenID();
    return kCommSucc;
}

// 配置机器人ID 初始化后不在改变,不需要锁保护
UserID RobotNet::GetUserID() const {
    return userid_;
}

TimeStamp RobotNet::GetTimeStamp() const {
    return timestamp_;
}

void RobotNet::SetTimeStamp(const TimeStamp& val) {
    timestamp_ = val;
}

int RobotNet::SnapShotObjectStatus() const {
#ifdef _DEBUG

    if (connection_) {
        LOG_INFO("token = %x", connection_->GetTokenID());
    }
    LOG_INFO("game ip [%s]", game_ip_.c_str());
    LOG_INFO("game port [%d]", game_port_);
    LOG_INFO("userid [%d]", userid_);
    LOG_INFO("token [%d]", connection_->GetTokenID());
#endif
    return kCommSucc;
}
