#pragma once
#include "robot_define.h"

class DepositHttpManager : public ISingletion<DepositHttpManager> {
public:
    // ��WithLock��׺�������������ж��߳̿ɼ�����,һ�㶼��Ҫ����������ҵ�����������
    // ��� lock + WithLock ������϶���, ��֤�����ö��mutex��std::mutex Ϊ�ǵݹ���
    // ֻ���ⲿ�߳̿ɼ� ���߳� �� �����߳�
    int Init();

    int Term();

    // ���� ��������״̬
    int SetDepositTypeAmount(const UserID& userid, const DepositType& type, const int64_t& amount);

    // ����״̬����
    int SnapShotObjectStatus();

    // ���ڲ����Ļ����˼���COPY  �����ر�mutex������������
    DepositMap GetDepositMap() const;

protected:
    SINGLETION_CONSTRUCTOR(DepositHttpManager);

private:
    // ��̨ ������ʱ��
    int ThreadDeposit();

    // ��̨ ����
    int RobotGainDeposit(const UserID& userid, const int64_t& amount) const;

    // ��̨ ����
    int RobotBackDeposit(const UserID& userid, const int64_t& amount) const;

    // ��̨ ������Ҫ�����������û�����
    int SendDepositRequest();

private:
    // ��̨���� ������
    mutable std::mutex mutex_;

    // ��̨���� ����
    DepositMap deposit_map_;

    // ��̨���� ��ʱ���߳�
    YQThread timer_thread_;

};

#define DepositHttpMgr DepositHttpManager::Instance()