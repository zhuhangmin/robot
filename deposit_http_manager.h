#pragma once
#include "robot_define.h"

class DepositHttpManager : public ISingletion<DepositHttpManager> {
public:
    // 无WithLock后缀函数：方法中有多线程可见数据,一般都需要加锁，除非业务层次允许脏读
    // 组合 lock + WithLock 函数组合而成, 保证不会获得多次mutex，std::mutex 为非递归锁
    // 只对外部线程可见 主线程 和 启动线程
    int Init();

    int Term();

    // 设置 补银还银状态
    int SetDepositTypeAmount(const UserID& userid, const DepositType& type, const int64_t& amount);

    // 对象状态快照
    int SnapShotObjectStatus();

    // 正在补银的机器人集合COPY  不返回被mutex保护对象引用
    DepositMap GetDepositMap() const;

protected:
    SINGLETION_CONSTRUCTOR(DepositHttpManager);

private:
    // 后台 补银定时器
    int ThreadDeposit();

    // 后台 补银
    int RobotGainDeposit(const UserID& userid, const int64_t& amount) const;

    // 后台 还银
    int RobotBackDeposit(const UserID& userid, const int64_t& amount) const;

    // 后台 操作需要补银还银的用户集合
    int SendDepositRequest();

private:
    // 后台补银 数据锁
    mutable std::mutex mutex_;

    // 后台补银 任务
    DepositMap deposit_map_;

    // 后台补银 定时器线程
    YQThread timer_thread_;

};

#define DepositHttpMgr DepositHttpManager::Instance()