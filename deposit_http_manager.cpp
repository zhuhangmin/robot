#include "stdafx.h"
#include "deposit_http_manager.h"
#include "setting_manager.h"
#include "main.h"
#include "robot_utils.h"
#include "game_net_manager.h"

#define ROBOT_APPLY_DEPOSIT_KEY "zjPUYq9L36oA9zke"

int DepositHttpManager::Init() {
    LOG_INFO_FUNC("\t[START]");
    timer_thread_.Initial(std::thread([this] {this->ThreadDeposit(); }));
    return kCommSucc;
}


int DepositHttpManager::Term() {
    timer_thread_.Release();
    deposit_map_.clear();
    user_game_info_map_.clear();

    LOG_INFO_FUNC("[EXIT]");
    return kCommSucc;
}

int DepositHttpManager::ThreadDeposit() {
    LOG_INFO("\t[START] deposit thread [%d] started", GetCurrentThreadId());

    while (true) {
        const auto dwRet = WaitForSingleObject(g_hExitServer, SettingMgr.GetDepositInterval());
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }

        if (WAIT_TIMEOUT == dwRet) {
            if (!g_inited) continue;
            SendDepositRequest();

            /*  if (kCommSucc != RoomMgr.GetTableDepositRange(roomid, tableno, max, min)) {
                  ASSERT_FALSE;
                  }*/

        }
    }

    LOG_INFO("[EXIT] deposit thread [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}


int DepositHttpManager::SendDepositRequest() {
    DepositMap deposit_map_temp;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        deposit_map_temp = std::move(deposit_map_);
    }

    for (auto& kv : deposit_map_temp) {
        const auto userid = kv.first;
        const auto data = kv.second;
        const auto deposit_type = data.type;
        const auto amount = data.amount;

        if (deposit_type == DepositType::kGain) {
            if (kCommSucc != RobotGainDeposit(userid, amount)) {
                ASSERT_FALSE;
                continue;
            }
        }

        if (deposit_type == DepositType::kBack) {
            if (kCommSucc != RobotBackDeposit(userid, amount)) {
                ASSERT_FALSE;
                continue;
            }
        }
    }
    return kCommSucc;
}

int DepositHttpManager::RobotGainDeposit(const UserID& userid, const int64_t& amount) const {
    LOG_INFO("RobotGainDeposit BEG userid [%d] amount [%I64d]", userid, amount);
    CHECK_USERID(userid);
    if (amount <= 0) ASSERT_FALSE_RETURN;

    const auto game_id = SettingMgr.GetGameID();
    CHECK_GAMEID(game_id);

    auto active_id = SettingMgr.GetDepositActiveID();
    if (active_id.empty()) ASSERT_FALSE_RETURN;
    const auto szValue = active_id.c_str();

    auto url = SettingMgr.GetDepositGainUrl();
    if (url.empty()) ASSERT_FALSE_RETURN;
    const auto szHttpUrl = url.c_str();

    Json::Value root;
    Json::FastWriter fast_writer;
    root["GameId"] = Json::Value(game_id);
    root["ActId"] = Json::Value(szValue);
    root["UserId"] = Json::Value(userid);
    root["UserNickName"] = Json::Value("Robot");
    root["Num"] = Json::Value(amount);
    root["Operation"] = Json::Value(1);
    root["Device"] = Json::Value(xyConvertIntToStr(game_id) + "+" + xyConvertIntToStr(userid));
    root["Time"] = Json::Value((LONGLONG) time(NULL) * 1000);
    root["Sign"] = Json::Value(MD5String((root["ActId"].asString() + "|" + root["UserId"].asString() + "|" + root["Time"].asString() + "|" + ROBOT_APPLY_DEPOSIT_KEY).c_str()));

    CString strParam = fast_writer.write(root).c_str();
    CString strResult = RobotUtils::ExecHttpRequestPost(szHttpUrl, strParam);
    Json::Reader reader;
    Json::Value _root;
    if (!reader.parse((LPCTSTR) strResult, _root)) ASSERT_FALSE_RETURN;

    const auto ret_code = _root["Code"].asInt();
    if (0 != ret_code) {
        LOG_ERROR("userid  = [%d] gain deposit fail, code  = [%d], strResult =  [%s]", userid, _root["Code"].asInt(), strResult);
        //ASSERT_FALSE_RETURN
    }
    LOG_INFO("RobotGainDeposit END userid [%d] amount [%I64d]", userid, amount);
    return kCommSucc;
}

int DepositHttpManager::RobotBackDeposit(const UserID& userid, const int64_t& amount) const {
    LOG_INFO("RobotBackDeposit BEG userid [%d] amount [%I64d]", userid, amount);
    CHECK_USERID(userid);
    if (amount <= 0) ASSERT_FALSE_RETURN;

    const auto game_id = SettingMgr.GetGameID();
    CHECK_GAMEID(game_id);

    auto active_id = SettingMgr.GetDepositActiveID();
    if (active_id.empty()) ASSERT_FALSE_RETURN;
    const auto szValue = active_id.c_str();

    auto url = SettingMgr.GetDepositBackUrl();
    if (url.empty()) ASSERT_FALSE_RETURN;
    const auto szHttpUrl = url.c_str();

    Json::Value root;
    Json::FastWriter fast_writer;
    root["GameId"] = Json::Value(game_id);
    root["ActId"] = Json::Value(szValue);
    root["UserId"] = Json::Value(userid);
    root["UserNickName"] = Json::Value("Robot");
    root["Num"] = Json::Value(amount);
    root["Operation"] = Json::Value(2);
    root["Device"] = Json::Value(xyConvertIntToStr(game_id) + "+" + xyConvertIntToStr(userid));
    root["Time"] = Json::Value((LONGLONG) time(NULL) * 1000);
    root["Sign"] = Json::Value(MD5String((root["ActId"].asString() + "|" + root["UserId"].asString() + "|" + root["Time"].asString() + "|" + ROBOT_APPLY_DEPOSIT_KEY).c_str()));

    CString strParam = fast_writer.write(root).c_str();
    CString strResult = RobotUtils::ExecHttpRequestPost(szHttpUrl, strParam);
    Json::Reader reader;
    Json::Value _root;
    if (!reader.parse((LPCTSTR) strResult, _root)) ASSERT_FALSE_RETURN;

    if (_root["Code"].asInt() != 0) {
        LOG_ERROR("userid  = [%d] gain deposit fail, code  = [%d], strResult =  [%s]", userid, _root["Code"].asInt(), strResult);
        ASSERT_FALSE_RETURN
    }

    LOG_INFO("RobotBackDeposit END userid [%d] amount [%I64d]", userid, amount);
    return kCommSucc;
}

int DepositHttpManager::SetDepositType(const UserID& userid, const DepositType& type, const int64_t& amount) {
    // 处理游戏服务器断线 但业务后台正常的情况
    if (!GameMgr.IsGameDataInited()) return kExceptionGameDataNotInited;
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_USERID(userid);
    if (amount < 0) ASSERT_FALSE_RETURN;
    if (deposit_map_.find(userid) == deposit_map_.end()) {
        deposit_map_[userid] = {type, amount};
    }

    return kCommSucc;
}

DepositMap DepositHttpManager::GetDepositMap() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return deposit_map_;
}

int DepositHttpManager::SetUserGameInfo(const int& userid, USER_GAMEINFO_MB* info) {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_USERID(userid);
    user_game_info_map_[userid] = *info;
    return kCommSucc;
}

UserGameInfoMap DepositHttpManager::GetUserGameInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return user_game_info_map_;
}

int DepositHttpManager::SnapShotObjectStatus() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("deposit_timer_thread_ [%d]", timer_thread_.GetThreadID());
    LOG_INFO("deposit_map_ size [%d]", deposit_map_.size());
    for (auto& kv : deposit_map_) {
        const auto userid = kv.first;
        const auto status = kv.second;
        LOG_INFO("robot userid [%d] status [%d]", userid, status);
    }

    LOG_INFO("user_game_info_map_ size [%d]", user_game_info_map_.size());
    for (auto& kv : user_game_info_map_) {
        const auto userid = kv.first;
        const auto game_info = kv.second;
        LOG_INFO("robot userid [%d] deposit [%I64d]", userid, game_info.nDeposit);
    }

    return kCommSucc;
}