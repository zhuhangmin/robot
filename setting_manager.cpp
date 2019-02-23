#include "stdafx.h"
#include "setting_manager.h"
#include "Main.h"
#include "RobotUitls.h"

int SettingManager::Init() {
    if (!InitSetting()) {
        UWL_ERR("InitSetting() failed");
        assert(false);
        return kCommFaild;
    }
    UWL_INF("InitSetting Account Count = %d", account_setting_map_.size());

    return kCommSucc;
}

bool SettingManager::InitSetting() {
    std::string filename = g_curExePath + ROBOT_SETTING_FILE_NAME;
    Json::Value root;
    Json::Reader reader;
    std::ifstream ifile;
    ifile.open(filename, std::ifstream::in);

    auto closeRet = [&ifile] (bool ret) {ifile.close(); return ret; };

    if (!reader.parse(ifile, root, FALSE))		return closeRet(false);

    if (!root.isMember("GameId"))               return closeRet(false);

    if (!root["RoomIds"].isArray())				return closeRet(false);

    if (!root["robots"].isArray())				return closeRet(false);

    g_gameID = root["GameId"].asInt();

    auto rooms = root["RoomIds"];
    int size = rooms.size();

    for (int n = 0; n < size; ++n) {
        int32_t nRoomId = rooms[n]["RoomId"].asInt();
        int32_t nCtrlMode = rooms[n]["CtrlMode"].asInt();
        int32_t nCtrlValue = rooms[n]["Value"].asInt();
        room_setting_map_[nRoomId] = RoomSetiing{nRoomId, nCtrlMode, nCtrlValue};
    }

    auto robots = root["robots"];
    size = robots.size();
    for (int i = 0; i < size; ++i) {
        auto robot = robots[i];
        int32_t		nAccount = robot["Account"].asInt();
        std::string sPassword = robot["Password"].asString();
        std::string sNickName = robot["NickName"].asString();
        std::string sPortrait = robot["Portrait"].asString();

        RobotSetting unit;
        unit.userid = nAccount;
        unit.password = sPassword;
        unit.nickName = sNickName;
        unit.portraitUrl = sPortrait;
        account_setting_map_[nAccount] = unit;
    }
    return closeRet(true);
}

int SettingManager::GetRobotSetting(int account, RobotSetting& robot_setting_) {
    if (account == 0)
        return kCommFaild;

    auto it = account_setting_map_.find(account);
    if (it == account_setting_map_.end())
        return kCommFaild;

    robot_setting_ = it->second;

    return kCommSucc;
}

int SettingManager::GetRandomRobotSetting(RobotSetting& robot_setting_) {
    auto random_pos = 0;
    if (kCommFaild == RobotUitls::GenRandInRange(0, account_setting_map_.size() - 1, random_pos)) {
        return kCommFaild;
    }
    auto random_it = std::next(std::begin(account_setting_map_), random_pos);
    robot_setting_ = random_it->second;
    return kCommSucc;
}

SettingManager::AccountSettingMap& SettingManager::GetAccountSettingMap() {
    return account_setting_map_;
}

SettingManager::RoomSettingMap& SettingManager::GetRoomSettingMap() {
    return room_setting_map_;
}
