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
    // 后台 补银定时器
    void ThreadDeposit();

private:
    // 具体业务

    // 后台 补银
    int RobotGainDeposit(UserID userid, int amount);

    // 后台 还银
    int RobotBackDeposit(UserID userid, int amount);

private:
    // 辅助函数 WithLock 标识调用前需要获得此对象的数据锁mutex
    int GetDepositTypeWithLock(const UserID& userid, DepositType& type);

    int SetDepositTypesWithLock(const UserID userid, DepositType type);

private:
    // 后台补银 数据锁
    std::mutex deposit_map_mutex_;

    // 后台补银 任务
    DepositMap deposit_map_;

    // 后台补银 定时器线程
    YQThread	deposit_timer_thread_;

};

#define DepositMgr RobotDepositManager::Instance()