#pragma once
#include "robot_define.h"

// ҵ���߼��������Դ˶���Ϊ׼��
class DepositDataManager : public ISingletion<DepositDataManager> {
public:
    int Init();

    int Term();

    // �û��Ƹ���ϢCOPY �����ر�mutex������������  
    UserDepositMap GetUserDepositMap() const;

    int GetDeposit(const int& userid, int64_t& deposit);

    int SetDeposit(const int& userid, const int64_t& deposit);

    int AddDeposit(const int& userid, const int64_t& amount);

public:
    int SnapShotObjectStatus();

private:

    int GetDepositWithLock(const int& userid, int64_t& deposit);

    int SetDepositWithLock(const int& userid, const int64_t& deposit);

protected:
    SINGLETION_CONSTRUCTOR(DepositDataManager);

private:
    // ������
    mutable std::mutex mutex_;

    // �û��Ƹ���Ϣ
    UserDepositMap user_game_info_map_;
};

#define DepositDataMgr DepositDataManager::Instance()