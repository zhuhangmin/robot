#pragma once
#include "robot_define.h"
class SettingManager : public ISingletion<SettingManager> {
public:
    int Init();

    int Term();

public:
    // ��ϷID
    GameID GetGameID() const;

    // �������˺�����
    const RobotSettingMap& GetRobotSettingMap() const;

    // �����˷�������
    const RoomSettingMap& GetRoomSettingMap() const;

    // ���ָ������������
    int GetRobotSetting(const UserID userid, RobotSetting& robot_setting) const;

    // ��ҵ��ʱ�����
    int GetMainsInterval() const;

    // ������ʱ�����
    int GetDepositInterval() const;

    // ÿ�β�������
    int GetGainAmount() const;

    // ÿ�λ�������
    int GetBackAmount() const;

public:
    // ����״̬����
    int SnapShotObjectStatus() const;

protected:
    SINGLETION_CONSTRUCTOR(SettingManager);

private:
    int InitSetting();

private:
    RobotSettingMap robot_setting_map_;

    RoomSettingMap room_setting_map_;

    GameID game_id_{InvalidGameID};

    int main_interval_{MainInterval};

    int deposit_interval_{DepositInterval};

    int gain_amount_{GainAmount};

    int back_amount_{BackAmount};

    //TODO HOT UPDATE NEED RESET AND LOCK?
};

#define SettingMgr  SettingManager::Instance()