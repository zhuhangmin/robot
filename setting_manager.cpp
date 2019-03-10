#include "stdafx.h"
#include "setting_manager.h"
#include "main.h"
#include "robot_utils.h"

int SettingManager::Init() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO_FUNC("[SVR START]");
    if (kCommSucc != InitSettingWithLock()) {
        LOG_ERROR("InitSetting() failed");
        ASSERT_FALSE_RETURN;
    }

    hot_update_thread_.Initial(std::thread([this] {this->ThreadHotUpdate(); }));

    return kCommSucc;
}

int SettingManager::Term() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO_FUNC("[EXIT ROUTINE]");
    return kCommSucc;
}

int SettingManager::ThreadHotUpdate() {
    LOG_INFO("[SVR START] SettingManager HotUpdate thread [%d] started", GetCurrentThreadId());
    while (true) {
        const auto dwRet = WaitForSingleObject(g_hExitServer, HotUpdateInterval);
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }
        if (WAIT_TIMEOUT == dwRet) {
            std::lock_guard<std::mutex> lock(mutex_);
            InitSettingWithLock();
        }
    }
    LOG_INFO("[EXIT ROUTINE] SettingManager HotUpdate thread [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}

int SettingManager::InitSettingWithLock() {
    const auto filename = g_curExePath + _T("robot.setting");
    Json::Value root;
    Json::Reader reader;
    std::ifstream ifile; //@zhuhangmin 20190226 RAII OBJ, NO NEED TO CLOSE
    ifile.open(filename, std::ifstream::in);

    if (!reader.parse(ifile, root, false))		ASSERT_FALSE_RETURN;

    if (!root.isMember("game_id"))               ASSERT_FALSE_RETURN;

    if (!root["rooms"].isArray())				ASSERT_FALSE_RETURN;

    if (!root["robots"].isArray())				ASSERT_FALSE_RETURN;

    game_id_ = root["game_id"].asInt();

    auto rooms = root["rooms"];
    int size = rooms.size();

    for (auto n = 0; n < size; ++n) {
        const auto roomid = rooms[n]["roomid"].asInt();
        const auto count = rooms[n]["count"].asInt();
        room_setting_map_[roomid] = RoomSetiing{roomid, count};
    }


    auto robots = root["robots"];
    size = robots.size();
    for (auto i = 0; i < size; ++i) {
        auto robot = robots[i];
        auto userid = static_cast<UserID>(robot["userid"].asInt());
        const auto password = robot["password"].asString();
        const auto nickname = robot["nickname"].asString();
        const auto headurl = robot["headurl"].asString();

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

    if (root.isMember("deposit_active_id"))
        deposit_active_id_ = root["deposit_active_id"].asString();

    if (root.isMember("deposit_gain_amount"))
        gain_amount_ = root["deposit_gain_amount"].asInt();

    if (root.isMember("deposit_back_amount"))
        back_amount_ = root["deposit_back_amount"].asInt();

    if (root.isMember("deposit_gain_url"))
        deposit_gain_url_ = root["deposit_gain_url"].asString();

    if (root.isMember("deposit_back_url"))
        deposit_back_url_ = root["deposit_back_url"].asString();

    if (root.isMember("deposit_init_gain"))
        deposit_init_gain_ = root["deposit_init_gain"].asInt();

    // check
    if (InvalidGameID == game_id_) {
        LOG_ERROR("game_id_  = [%d]", InvalidGameID);
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

std::string& SettingManager::GetDepositActiveID() {
    std::lock_guard<std::mutex> lock(mutex_);
    return deposit_active_id_;
}

const RobotSettingMap& SettingManager::GetRobotSettingMap() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return robot_setting_map_;
}

const RoomSettingMap& SettingManager::GetRoomSettingMap() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return room_setting_map_;
}

int SettingManager::GetRobotSetting(const UserID userid, RobotSetting& robot_setting) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto iter = robot_setting_map_.find(userid);
    if (iter == robot_setting_map_.end()) {
        ASSERT_FALSE_RETURN;
    }
    robot_setting = iter->second;
    return kCommSucc;
}

int SettingManager::GetMainsInterval() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return main_interval_;
}

int SettingManager::GetDepositInterval() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return deposit_interval_;
}

int SettingManager::GetGainAmount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return gain_amount_;
}

int SettingManager::GetBackAmount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return back_amount_;
}

GameID SettingManager::GetGameID() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return game_id_;
}

std::string& SettingManager::GetDepositGainUrl() {
    std::lock_guard<std::mutex> lock(mutex_);
    return deposit_gain_url_;
}

std::string& SettingManager::GetDepositBackUrl() {
    std::lock_guard<std::mutex> lock(mutex_);
    return deposit_back_url_;
}

int SettingManager::GetRandomUserID(UserID& random_userid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    // Ëæ»úÑ¡È¡userid
    auto random_pos = 0;
    if (kCommSucc != RobotUtils::GenRandInRange(0, robot_setting_map_.size() - 1, random_pos)) {
        ASSERT_FALSE_RETURN;
    }
    const auto random_it = std::next(std::begin(robot_setting_map_), random_pos);
    random_userid = random_it->first;
    return kCommSucc;
}

int SettingManager::GetDeposiInitGainFlag() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return deposit_init_gain_;
}


int SettingManager::SnapShotObjectStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("game_id_ [%d]", game_id_);
    LOG_INFO("main_interval_ [%d]", main_interval_);
    LOG_INFO("deposit_interval_ [%d]", deposit_interval_);
    LOG_INFO("gain_amount_ [%d]", gain_amount_);
    LOG_INFO("back_amount_ [%d]", back_amount_);
    LOG_INFO("deposit_active_id_ [%s]", deposit_active_id_.c_str());

    LOG_INFO("robot_setting_map_ size [%d]", robot_setting_map_.size());
    std::string userid_str;
    for (auto& kv : robot_setting_map_) {
        const auto userid = kv.first;
        userid_str += std::to_string(userid);
        userid_str += ", ";
    }

    LOG_INFO("room_setting_map_ size [%d]", room_setting_map_.size());
    for (auto& kv : room_setting_map_) {
        const auto roomid = kv.first;
        const auto setting = kv.second;
        LOG_INFO("roomid [%d]  count [%d]", roomid, setting.count);
    }

    return kCommSucc;
}

int SettingManager::BriefInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("robot_setting_map_ size [%d]", robot_setting_map_.size());
    return kCommSucc;
}