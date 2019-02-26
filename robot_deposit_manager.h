#pragma once
#include "robot_define.h"

class RobotDepositManager :public ISingletion<RobotDepositManager> {
public:
    int Init();

    int Term();

protected:
    SINGLETION_CONSTRUCTOR(RobotDepositManager);

private:
    // ��̨ ������ʱ��
    int ThreadDeposit();

private:
    // ����ҵ��

    // ��̨ ����
    int RobotGainDeposit(const UserID userid, const int amount) const;

    // ��̨ ����
    int RobotBackDeposit(const UserID userid, const int amount) const;

private:
    // �������� WithLock ��ʶ����ǰ��Ҫ��ô˶����������mutex
    int GetDepositTypeWithLock(const UserID& userid, DepositType& type) const;

    int SetDepositTypesWithLock(const UserID userid, const DepositType type);

private:
    // ��̨���� ������
    mutable std::mutex deposit_map_mutex_;

    // ��̨���� ����
    DepositMap deposit_map_;

    // ��̨���� ��ʱ���߳�
    YQThread	deposit_timer_thread_;

};

#define DepositMgr RobotDepositManager::Instance()