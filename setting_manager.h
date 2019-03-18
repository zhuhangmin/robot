#pragma once
#include "robot_define.h"
class SettingManager : public ISingletion<SettingManager> {
public:
    int Init();

    int Term() const;

public:
    // ��ϷID
    GameID GetGameID() const;

    // �������˺����� ֧���ȸ��£����ṩ��mutex���������ã�����copy
    RobotSettingMap GetRobotSettingMap() const;

    // �����˷������� ֧���ȸ��£����ṩ��mutex���������ã�����copy
    RoomSettingMap GetRoomSettingMap() const;

    // ���ָ������������ ֧���ȸ��£����ṩ��mutex���������ã�����copy
    RobotSetting GetRobotSetting(const UserID& userid) const;

    // ��ҵ��ʱ�����
    int GetMainsInterval() const;

    // ������ʱ�����
    int GetDepositInterval() const;

    // �����ID
    std::string& GetDepositActiveID();

    // ÿ�β�������
    int64_t GetGainAmount() const;

    // ÿ�λ�������
    int64_t GetBackAmount() const;

    // ����url
    std::string& GetDepositGainUrl();

    // ����url
    std::string& GetDepositBackUrl();

    // �ȸ���
    int ThreadHotUpdate();

    // ��鷿�������Ƿ����
    int IsRoomSettingExist(const RoomID& roomid, bool& exist);

    // ��鷿�������Ƿ����
    int IsRobotSettingExist(const UserID& userid, bool& exist);

public:
    // ����״̬����
    int SnapShotObjectStatus() const;

    int BriefInfo() const;

    // ������userid
    int GetRandomUserID(UserID& random_userid) const;

    // ����ʱ�Ƿ��岹��
    int GetDeposiInitGainFlag() const;

protected:
    SINGLETION_CONSTRUCTOR(SettingManager);

private:
    int InitSettingWithLock();

private:
    mutable std::mutex mutex_;

    YQThread timer_thread_;

    RobotSettingMap robot_setting_map_;

    RoomSettingMap room_setting_map_;

    GameID game_id_{InvalidGameID};

    int main_interval_{MainInterval};

    int deposit_interval_{DepositInterval};

    int deposit_init_gain_{InitGainFlag};

    int64_t gain_amount_{GainAmount};

    int64_t back_amount_{BackAmount};

    std::string deposit_active_id_;

    std::string deposit_gain_url_;

    std::string deposit_back_url_;
};

#define SettingMgr  SettingManager::Instance()