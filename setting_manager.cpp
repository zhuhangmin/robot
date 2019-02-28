#include "stdafx.h"
#include "setting_manager.h"
#include "main.h"
#include "robot_utils.h"

int SettingManager::Init() {
    LOG_FUNC("[START ROUTINE]");
    if (kCommSucc != InitSetting()) {
        UWL_ERR("InitSetting() failed");
        ASSERT_FALSE;
        return kCommFaild;
    }
    UWL_INF("InitSetting robot Count = %d", robot_setting_map_.size());

    return kCommSucc;
}

int SettingManager::Term() {
    LOG_FUNC("[EXIT ROUTINE]");
    return kCommSucc;
}

int SettingManager::InitSetting() {
    LOG_FUNC("[START ROUTINE]");
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
        int count = rooms[n]["count"].asInt();
        room_setting_map_[roomid] = RoomSetiing{roomid, count};
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
        ASSERT_FALSE;
        return kCommFaild;
    }

    return kCommSucc;
}

const RobotSettingMap& SettingManager::GetRobotSettingMap() const {
    return robot_setting_map_;
}

const RoomSettingMap& SettingManager::GetRoomSettingMap() const {
    return room_setting_map_;
}

int SettingManager::GetRobotSetting(const UserID userid, RobotSetting& robot_setting) const {
    auto& iter = robot_setting_map_.find(userid);
    if (iter == robot_setting_map_.end()) {
        return kCommFaild;
    }
    robot_setting = iter->second;
    return kCommSucc;
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

int SettingManager::SnapShotObjectStatus() const {
    LOG_FUNC("[SNAPSHOT] BEG");

    LOG_INFO("OBJECT ADDRESS = %x", this);
    LOG_INFO("game_id_ [%d]", game_id_);
    LOG_INFO("main_interval_ [%d]", main_interval_);
    LOG_INFO("deposit_interval_ [%d]", deposit_interval_);
    LOG_INFO("gain_amount_ [%d]", gain_amount_);
    LOG_INFO("back_amount_ [%d]", back_amount_);

    LOG_INFO("robot_setting_map_ size [%d]", robot_setting_map_.size());
    std::string userid_str;
    for (auto& kv : robot_setting_map_) {
        auto userid = kv.first;
        userid_str += std::to_string(userid);
        userid_str += ", ";
    }
    LOG_INFO("robot userid [%s]", userid_str.c_str());

    LOG_INFO("room_setting_map_ size [%d]", room_setting_map_.size());
    for (auto& kv : room_setting_map_) {
        auto roomid = kv.first;
        auto setting = kv.second;
        LOG_INFO("roomid [%d]  count [%d]", roomid, setting.count);
    }

    LOG_FUNC("[SNAPSHOT] END");
    return kCommSucc;
}



