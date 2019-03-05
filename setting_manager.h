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
    int GetRobotSetting(UserID userid, RobotSetting& robot_setting) const;

    // ��ҵ��ʱ�����
    int GetMainsInterval() const;

    // ������ʱ�����
    int GetDepositInterval() const;

    // �����ID
    std::string& GetDepositActiveID();

    // ÿ�β�������
    int GetGainAmount() const;

    // ÿ�λ�������
    int GetBackAmount() const;

    // ����url
    std::string& GetDepositGainUrl();

    // ����url
    std::string& GetDepositBackUrl();

public:
    // ����״̬����
    int SnapShotObjectStatus() const;

    // ������userid
    int GetRandomUserID(UserID& random_userid) const;

    // ����ʱ�Ƿ��岹��
    int GetDeposiInitGainFlag() const;

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

    int deposit_init_gain_{InitGainFlag};

    int gain_amount_{GainAmount};

    int back_amount_{BackAmount};

    std::string deposit_active_id_;

    std::string deposit_gain_url_;

    std::string deposit_back_url_;



    //TODO HOT UPDATE NEED RESET AND LOCK?
};

#define SettingMgr  SettingManager::Instance()