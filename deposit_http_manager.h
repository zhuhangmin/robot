#pragma once
#include "robot_define.h"

class DepositHttpManager :public ISingletion<DepositHttpManager> {
public:
    int Init();

    int Term();

    // ���� ��������״̬
    int SetDepositType(const UserID& userid, const DepositType& type, const int64_t& amount);

    // ����״̬����
    int SnapShotObjectStatus();

    // �����û��Ƹ���Ϣ
    int SetUserGameInfo(const int& userid, USER_GAMEINFO_MB* info);

    // �û��Ƹ���ϢCOPY  �����ر�mutex������������
    UserGameInfoMap GetUserGameInfo() const;

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

    // �û��Ƹ���Ϣ
    UserGameInfoMap user_game_info_map_;

};

#define DepositHttpMgr DepositHttpManager::Instance()