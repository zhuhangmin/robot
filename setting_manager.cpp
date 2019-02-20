#include "stdafx.h"
#include "setting_manager.h"
#include "Main.h"

bool	SettingManager::Init() {
    if (!InitSetting()) {
        UWL_ERR("InitSetting() return false");
        assert(false);
        return false;
    }
    UWL_INF("InitSetting Account Count = %d", account_setting_map_.size());

    return true;
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
        unit.account = nAccount;
        unit.password = sPassword;
        unit.nickName = sNickName;
        unit.portraitUrl = sPortrait;
        account_setting_map_[nAccount] = unit;
    }
    return closeRet(true);
}

bool SettingManager::GetRobotSetting(int account, RobotSetting& unit) {
    if (account == 0)	return false;

    auto it = account_setting_map_.find(account);
    if (it == account_setting_map_.end()) return false;

    unit = it->second;

    return true;
}
