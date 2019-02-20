#pragma once
#include "RobotDef.h"
class SettingManager : public ISingletion<SettingManager> {
public:
    using RoomSettingMap = std::unordered_map<RoomID, RoomSetiing>;
    using AccountSettingMap = std::unordered_map<UserID, RobotSetting>;
public:
    int Init();

    // ���ָ������������
    int GetRobotSetting(int account, RobotSetting& robot_setting_);

    int GetRandomRobotSetting(RobotSetting& robot_setting_);

    AccountSettingMap& GetAccountSettingMap();
protected:
    SINGLETION_CONSTRUCTOR(SettingManager);

    bool InitSetting();
private:
    // �������˺�����  robot.setting
    AccountSettingMap	account_setting_map_;

    // �����˷������� robot.setting
    RoomSettingMap room_setting_map_;

};

