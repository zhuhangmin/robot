#pragma once
#include "robot_define.h"
class SettingManager : public ISingletion<SettingManager> {
public:
    using RoomSettingMap = std::unordered_map<RoomID, RoomSetiing>;
    using RobotSettingMap = std::unordered_map<UserID, RobotSetting>;
public:
    int Init();

public:
    // ���ָ������������
    int GetRobotSetting(UserID userid, RobotSetting& robot_setting_);

    RobotSettingMap& GetRobotSettingMap();

    RoomSettingMap& GetRoomSettingMap();

    GameID GetGameID() const;

    std::string& GetGameIP() { return game_ip_; }

    int GetMainsInterval() const { return main_interval_; }

    int GetDepositInterval() const { return deposit_interval_; }

    int GetGainAmount() const { return gain_amount_; }

    int GetBackAmount() const { return back_amount_; }

protected:
    SINGLETION_CONSTRUCTOR(SettingManager);

    bool InitSetting();
private:
    // �������˺�����
    RobotSettingMap	robot_setting_map_;

    // �����˷�������
    RoomSettingMap room_setting_map_;

    GameID game_id_{InvalidGameID};

    std::string game_ip_ = std::string("127.0.0.1");

    int main_interval_{MainInterval};

    int deposit_interval_{DepositInterval};

    int gain_amount_{GainAmount};

    int back_amount_{BackAmount};

};

