#pragma once
#include "robot_define.h"
class DepositDataManager : public ISingletion<DepositDataManager> {
public:
    int Init();

    int Term();

    // 设置用户财富信息
    int SetUserGameData(const int& userid, USER_GAMEINFO_MB* info);

    // 用户财富信息COPY  不返回被mutex保护对象引用
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
    // 数据锁
    mutable std::mutex mutex_;

    // 用户财富信息
    UserGameInfoMap user_game_info_map_;
};

#define DepositDataMgr DepositDataManager::Instance()