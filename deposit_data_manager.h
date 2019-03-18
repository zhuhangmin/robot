#pragma once
#include "robot_define.h"
class DepositDataManager : public ISingletion<DepositDataManager> {
public:
    int Init();

    int Term();

    // �����û��Ƹ���Ϣ
    int SetUserGameData(const int& userid, USER_GAMEINFO_MB* info);

    // �û��Ƹ���ϢCOPY  �����ر�mutex������������
    UserGameInfoMap GetUserGameDataMap() const;

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
    UserGameInfoMap user_game_info_map_;
};

#define DepositDataMgr DepositDataManager::Instance()