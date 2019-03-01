#pragma once
#include "robot_define.h"

class RobotDepositManager :public ISingletion<RobotDepositManager> {
public:
    int Init();

    int Term();

public:
    // 设置 补银还银状态
    int SetDepositType(const UserID userid, const DepositType type);

public:
    // 对象状态快照
    int SnapShotObjectStatus();

protected:
    SINGLETION_CONSTRUCTOR(RobotDepositManager);

private:
    // 后台 补银定时器
    int ThreadDeposit();

    // 后台 补银
    int RobotGainDeposit(const UserID userid, const int amount) const;

    // 后台 还银
    int RobotBackDeposit(const UserID userid, const int amount) const;

private:
    // 后台补银 数据锁
    mutable std::mutex deposit_map_mutex_;

    // 后台补银 任务
    DepositMap deposit_map_;

    // 后台补银 定时器线程
    YQThread	deposit_timer_thread_;



};

#define DepositMgr RobotDepositManager::Instance()