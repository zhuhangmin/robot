#pragma once
#include "robot_define.h"

class RobotDepositManager :public ISingletion<RobotDepositManager> {
public:
    int Init();

    int Term();

protected:
    SINGLETION_CONSTRUCTOR(RobotDepositManager);

private:
    // 后台 补银定时器
    int ThreadDeposit();

private:
    // 具体业务

    // 后台 补银
    int RobotGainDeposit(const UserID userid, const int amount) const;

    // 后台 还银
    int RobotBackDeposit(const UserID userid, const int amount) const;

private:
    // 辅助函数 WithLock 标识调用前需要获得此对象的数据锁mutex
    int GetDepositTypeWithLock(const UserID& userid, DepositType& type) const;

    int SetDepositTypesWithLock(const UserID userid, const DepositType type);

private:
    // 后台补银 数据锁
    mutable std::mutex deposit_map_mutex_;

    // 后台补银 任务
    DepositMap deposit_map_;

    // 后台补银 定时器线程
    YQThread	deposit_timer_thread_;

};

#define DepositMgr RobotDepositManager::Instance()