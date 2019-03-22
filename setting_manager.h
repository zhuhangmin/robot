#pragma once
#include "robot_define.h"

// ��֧���ȸ�����ֻ��immutable����,���Ա�֤���̰߳�ȫ
// һ��֧���ȸ��� ��Ҫ���Ǽ���������ɼ���Σ��ϲ㻺��ʧЧ������
class SettingManager : public ISingletion<SettingManager> {
public:
    int Init();

    int Term() const;

public:
    // ��ϷID
    GameID GetGameID() const;

    // �������˺����� 
    RobotSettingMap GetRobotSettingMap() const;

    // �����˷������� 
    RoomSettingMap GetRoomSettingMap() const;

    // ���ָ������������ 
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

    // ��ʱ��Ϣ
    int ThreadTimer();

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

protected:
    SINGLETION_CONSTRUCTOR(SettingManager);

private:
    int InitSettingWithLock();

private:
    // ����������
    RobotSettingMap robot_setting_map_;

    // �����˷�������
    RoomSettingMap room_setting_map_;

    // ��ϷID
    GameID game_id_{InvalidGameID};

    // ҵ�����̶߳�ʱ�����
    int main_interval_{MainInterval};

    // �������̶߳�ʱ�����
    int deposit_interval_{DepositInterval};

    // Ĭ�ϵ��β�������
    int64_t gain_amount_{GainAmount};

    // Ĭ�ϵ��λ�������
    int64_t back_amount_{BackAmount};

    // ��̨���ò��������ID
    std::string deposit_active_id_;

    // ��̨���������ַ
    std::string deposit_gain_url_;

    // ��̨���������ַ
    std::string deposit_back_url_;
};

#define SettingMgr  SettingManager::Instance()