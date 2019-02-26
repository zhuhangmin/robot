#include "stdafx.h"
#include "setting_manager.h"
#include "main.h"
#include "robot_utils.h"

int SettingManager::Init() {
    if (kCommSucc != InitSetting()) {
        UWL_ERR("InitSetting() failed");
        assert(false);
        return kCommFaild;
    }
    UWL_INF("InitSetting robot Count = %d", robot_setting_map_.size());

    return kCommSucc;
}

void SettingManager::Term() {

}

int SettingManager::InitSetting() {
    std::string filename = g_curExePath + _T("robot.setting");
    Json::Value root;
    Json::Reader reader;
    std::ifstream ifile; //@zhuhangmin 20190226 RAII OBJ, NO NEED TO CLOSE
    ifile.open(filename, std::ifstream::in);

    if (!reader.parse(ifile, root, false))		return kCommFaild;

    if (!root.isMember("game_id"))               return kCommFaild;

    if (!root["rooms"].isArray())				return kCommFaild;

    if (!root["robots"].isArray())				return kCommFaild;

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
        return kCommFaild;
    }

    return kCommSucc;
}

int SettingManager::GetRobotSetting(UserID userid, RobotSetting& robot_setting_) {
    CHECK_USERID(userid);
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

int SettingManager::GetMainsInterval() const {
    return main_interval_;
}

int SettingManager::GetDepositInterval() const {
    return deposit_interval_;
}

int SettingManager::GetGainAmount() const {
    return gain_amount_;
}

int SettingManager::GetBackAmount() const {
    return back_amount_;
}

GameID SettingManager::GetGameID() const {
    return game_id_;
}

