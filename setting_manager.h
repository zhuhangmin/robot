#pragma once
#include "robot_define.h"
class SettingManager : public ISingletion<SettingManager> {
public:
    using RoomSettingMap = std::unordered_map<RoomID, RoomSetiing>;
    using RobotSettingMap = std::unordered_map<UserID, RobotSetting>;
public:
    int Init();

    void Term();

public:
    // 获得指定机器人配置
    int GetRobotSetting(UserID userid, RobotSetting& robot_setting_);

    RobotSettingMap& GetRobotSettingMap();

    RoomSettingMap& GetRoomSettingMap();

    GameID GetGameID() const;

    int GetMainsInterval() const { return main_interval_; }

    int GetDepositInterval() const { return deposit_interval_; }

    int GetGainAmount() const { return gain_amount_; }

    int GetBackAmount() const { return back_amount_; }

protected:
    SINGLETION_CONSTRUCTOR(SettingManager);

    bool InitSetting();
private:
    // 机器人账号配置
    RobotSettingMap	robot_setting_map_;

    // 机器人房间配置
    RoomSettingMap room_setting_map_;

    GameID game_id_{InvalidGameID};

    int main_interval_{MainInterval};

    int deposit_interval_{DepositInterval};

    int gain_amount_{GainAmount};

    int back_amount_{BackAmount};

    //TODO HOT UPDATE NEED RESET AND LOCK?
};

#define SettingMgr  SettingManager::Instance()