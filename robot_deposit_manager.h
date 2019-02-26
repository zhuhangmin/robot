#pragma once
#include "robot_define.h"
class RobotDepositManager :public ISingletion<RobotDepositManager> {
public:
    using DepositMap = std::unordered_map<UserID, DepositType>;

public:
    int Init();

    void Term();

protected:
    SINGLETION_CONSTRUCTOR(RobotDepositManager);

private:
    // ��̨ ������ʱ��
    void ThreadDeposit();

private:
    // ����ҵ��

    // ��̨ ����
    int RobotGainDeposit(UserID userid, int amount);

    // ��̨ ����
    int RobotBackDeposit(UserID userid, int amount);

private:
    // �������� WithLock ��ʶ����ǰ��Ҫ��ô˶����������mutex
    int GetDepositTypeWithLock(const UserID& userid, DepositType& type);

    int SetDepositTypesWithLock(const UserID userid, DepositType type);

private:
    // ��̨���� ������
    std::mutex deposit_map_mutex_;

    // ��̨���� ����
    DepositMap deposit_map_;

    // ��̨���� ��ʱ���߳�
    YQThread	deposit_timer_thread_;

};

#define DepositMgr RobotDepositManager::Instance()