#include "stdafx.h"
#include "setting_manager.h"
#include "main.h"
#include "robot_utils.h"

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
    std::string filename = g_curExePath + _T("robot.setting");
    Json::Value root;
    Json::Reader reader;
    std::ifstream ifile;
    ifile.open(filename, std::ifstream::in);

    auto close_ret = [&ifile] (bool ret) {ifile.close(); return ret; };

    if (!reader.parse(ifile, root, false))		return close_ret(false);

    if (!root.isMember("game_id"))               return close_ret(false);

    if (!root["rooms"].isArray())				return close_ret(false);

    if (!root["robots"].isArray())				return close_ret(false);

    game_id_ = root["game_id"].asInt();

    auto rooms = root["rooms"];
    int size = rooms.size();

    for (int n = 0; n < size; ++n) {
        RoomID roomid = rooms[n]["roomid"].asInt();
        EnterGameMode mode = (EnterGameMode) rooms[n]["mode"].asInt();
        int count = rooms[n]["count"].asInt();
        room_setting_map_[roomid] = RoomSetiing{roomid, mode, count};
    }

    auto robots = root["robots"];
    size = robots.size();
    for (int i = 0; i < size; ++i) {
        auto robot = robots[i];
        UserID		userid = (UserID) robot["userid"].asInt();
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

    // optional setting
    if (root.isMember("game_ip"))
        game_ip_ = root["game_ip"].asString();

    if (root.isMember("main_interval"))
        main_interval_ = root["main_interval"].asInt();

    if (root.isMember("deposit_interval"))
        deposit_interval_ = root["deposit_interval"].asInt();

    if (root.isMember("gain_amount"))
        gain_amount_ = root["gain_amount"].asInt();

    if (root.isMember("back_amount"))
        back_amount_ = root["back_amount"].asInt();


    // check
    if (InvalidGameID == game_id_) {
        UWL_ERR("game_id_ = %d", InvalidGameID);
        assert(false);
        return close_ret(false);
    }

    return close_ret(true);
}

int SettingManager::GetRobotSetting(UserID userid, RobotSetting& robot_setting_) {
    if (userid == 0) {
        UWL_ERR("userid = %d", userid);
        assert(false);
        return kCommFaild;
    }

    auto it = robot_setting_map_.find(userid);
    if (it == robot_setting_map_.end()) {
        assert(false);
        return kCommFaild;
    }


    robot_setting_ = it->second;

    return kCommSucc;
}

SettingManager::RobotSettingMap& SettingManager::GetRobotSettingMap() {
    return robot_setting_map_;
}

SettingManager::RoomSettingMap& SettingManager::GetRoomSettingMap() {
    return room_setting_map_;
}

GameID SettingManager::GetGameID() const {
    return game_id_;
}
