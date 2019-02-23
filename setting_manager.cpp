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
    UWL_INF("InitSetting robot Count = %d", robot_setting_map_.size());

    return kCommSucc;
}

bool SettingManager::InitSetting() {
    std::string filename = g_curExePath + ROBOT_SETTING_FILE_NAME;
    Json::Value root;
    Json::Reader reader;
    std::ifstream ifile;
    ifile.open(filename, std::ifstream::in);

    auto close_ret = [&ifile] (bool ret) {ifile.close(); return ret; };

    if (!reader.parse(ifile, root, FALSE))		return close_ret(false);

    if (!root.isMember("game_id"))               return close_ret(false);

    if (!root["rooms"].isArray())				return close_ret(false);

    if (!root["robots"].isArray())				return close_ret(false);

    game_id_ = root["game"].asInt();

    auto rooms = root["rooms"];
    int size = rooms.size();

    for (int n = 0; n < size; ++n) {
        RoomID roomid = rooms[n]["roomid"].asInt();
        EnterGameMode mode = (EnterGameMode) rooms[n]["mode"].asInt();
        int32_t count = rooms[n]["count"].asInt();
        room_setting_map_[roomid] = RoomSetiing{roomid, mode, count};
    }

    auto robots = root["robots"];
    size = robots.size();
    for (int i = 0; i < size; ++i) {
        auto robot = robots[i];
        int32_t		userid = robot["userid"].asInt();
        std::string password = robot["password"].asString();
        std::string nickname = robot["nickname"].asString();
        std::string headurl = robot["headurl"].asString();

        RobotSetting unit;
        unit.userid = userid;
        unit.password = password;
        unit.nickname = nickname;
        unit.headurl = headurl;
        robot_setting_map_[userid] = unit;
    }

    //optional setting
    if (root.isMember("game_ip"))
        game_ip_ = root["game_ip"].asString();

    return close_ret(true);
}

int SettingManager::GetRobotSetting(UserID userid, RobotSetting& robot_setting_) {
    if (userid == 0)
        return kCommFaild;

    auto it = robot_setting_map_.find(userid);
    if (it == robot_setting_map_.end())
        return kCommFaild;

    robot_setting_ = it->second;

    return kCommSucc;
}

SettingManager::RobotSettingMap& SettingManager::GetRobotSettingMap() {
    return robot_setting_map_;
}

SettingManager::RoomSettingMap& SettingManager::GetRoomSettingMap() {
    return room_setting_map_;
}
