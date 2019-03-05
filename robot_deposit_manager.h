#pragma once
#include "robot_define.h"

class RobotDepositManager :public ISingletion<RobotDepositManager> {
public:
    int Init();

    int Term();

    // ���� ��������״̬
    int SetDepositType(UserID userid, const DepositType& type);

    // ����״̬����
    int SnapShotObjectStatus();

protected:
    SINGLETION_CONSTRUCTOR(RobotDepositManager);

private:
    // ��̨ ������ʱ��
    int ThreadDeposit();

    // ��̨ ����
    int RobotGainDeposit(UserID userid, int amount) const;

    // ��̨ ����
    int RobotBackDeposit(UserID userid, int amount) const;

private:
    // ��̨���� ������
    mutable std::mutex deposit_map_mutex_;

    // ��̨���� ����
    DepositMap deposit_map_;

    // ��̨���� ��ʱ���߳�
    YQThread	deposit_timer_thread_;



};

#define DepositMgr RobotDepositManager::Instance()