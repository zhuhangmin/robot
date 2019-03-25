#pragma once
#include "robot_define.h"

// 业务逻辑中银子以此对象为准！
class DepositDataManager : public ISingletion<DepositDataManager> {
public:
    int Init();

    int Term();

    // 用户财富信息COPY 不返回被mutex保护对象引用  
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
    // 数据锁
    mutable std::mutex mutex_;

    // 用户财富信息
    UserDepositMap user_game_info_map_;
};

#define DepositDataMgr DepositDataManager::Instance()