#include "stdafx.h"
#include "robot_deposit_manager.h"
#include "setting_manager.h"
#include "main.h"
#include "robot_utils.h"

#define ROBOT_APPLY_DEPOSIT_KEY "zjPUYq9L36oA9zke"

int RobotDepositManager::Init() {
    LOG_FUNC("[START ROUTINE]");
    deposit_timer_thread_.Initial(std::thread([this] {this->ThreadDeposit(); }));
    return kCommSucc;
}


int RobotDepositManager::Term() {
    deposit_timer_thread_.Release();
    LOG_FUNC("[EXIT ROUTINE]");
    return kCommSucc;
}

int RobotDepositManager::ThreadDeposit() {
    LOG_INFO("[START ROUTINE] RobotDepositManager Deposit thread [%d] started", GetCurrentThreadId());

    while (true) {
        const auto dwRet = WaitForSingleObject(g_hExitServer, SettingMgr.GetDepositInterval());
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }

        if (WAIT_TIMEOUT == dwRet) {
            DepositMap deposit_map_temp;
            {
                std::lock_guard<std::mutex> lock(deposit_map_mutex_);
                deposit_map_temp = std::move(deposit_map_);
            }

            for (auto& kv : deposit_map_temp) {
                const auto userid = kv.first;
                const auto deposit_type = kv.second;

                if (deposit_type == DepositType::kGain) {
                    if (kCommSucc != RobotGainDeposit(userid, SettingMgr.GetGainAmount())) {
                        ASSERT_FALSE_RETURN;
                    }
                }

                if (deposit_type == DepositType::kBack) {
                    if (kCommSucc != RobotBackDeposit(userid, SettingMgr.GetBackAmount())) {
                        ASSERT_FALSE_RETURN;
                    }
                }
            }
        }
    }

    LOG_INFO("[EXIT ROUTINE] RobotDepositManager Deposit thread [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}

int RobotDepositManager::RobotGainDeposit(const UserID& userid, const int& amount) const {
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

    if (_root["Code"].asInt() != 0) {
        LOG_ERROR("userid  = [%d] gain deposit fail, code  = [%d], strResult =  [%s]", userid, _root["Code"].asInt(), strResult);
        ASSERT_FALSE_RETURN
    }
    return kCommSucc;
}

int RobotDepositManager::RobotBackDeposit(const UserID userid, const int amount) const {
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
    return kCommSucc;
}

int RobotDepositManager::SetDepositType(const UserID& userid, const DepositType& type) {
    CHECK_USERID(userid);
    std::lock_guard<std::mutex> lock(deposit_map_mutex_);
    if (deposit_map_.find(userid) == deposit_map_.end()) {
        deposit_map_[userid] = type;
    }

    return kCommSucc;
}

int RobotDepositManager::SnapShotObjectStatus() {
    std::lock_guard<std::mutex> lock(deposit_map_mutex_);
    LOG_INFO("OBJECT ADDRESS [%x]", this);
    LOG_INFO("deposit_timer_thread_ [%d]", deposit_timer_thread_.GetThreadID());
    LOG_INFO("deposit_map_ size [%d]", deposit_map_.size());
    for (auto& kv : deposit_map_) {
        const auto userid = kv.first;
        const auto status = kv.second;
        LOG_INFO("robot userid [%d] status [%d]", userid, status);
    }

    return kCommSucc;
}