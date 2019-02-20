#pragma once
#include "RobotDef.h"
class SettingManager : public ISingletion<SettingManager> {
public:
    using RoomSettingMap = std::unordered_map<RoomID, RoomSetiing>;
    using AccountSettingMap = std::unordered_map<UserID, RobotSetting>;
public:
    bool Init();
    // 机器人配置接口
    bool GetRobotSetting(int account, RobotSetting& robot_setting_);
    // 机器人数据管理
    uint32_t GetAccountSettingSize() { return account_setting_map_.size(); }
protected:
    SINGLETION_CONSTRUCTOR(SettingManager);

    bool InitSetting();
private:
    // 机器人账号配置  robot.setting
    AccountSettingMap	account_setting_map_;

    // 机器人房间配置 robot.setting
    RoomSettingMap room_setting_map_;
};

