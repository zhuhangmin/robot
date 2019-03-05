#pragma once
#include "robot_define.h"
#include "robot_net.h"

using RobotMap = std::unordered_map<UserID, RobotPtr>;

class RobotNetManager : public ISingletion<RobotNetManager> {
public:
    int Init();

    int Term();

    // 获得（不存在创建）指定机器人
    int GetRobotWithCreate(const UserID& userid, RobotPtr& robot);

    // 获得所有机器人接收消息的线程
    ThreadID GetRobotNotifyThreadID() const;

protected:
    SINGLETION_CONSTRUCTOR(RobotNetManager);

private:
    // 机器人 定时心跳
    int ThreadRobotPulse();

    // 机器人 消息接收
    int ThreadRobotNotify();

    // 机器人 消息处理
    int OnRobotNotify(const RequestID& requestid, void* ntf_data_ptr, const int& data_size, const TokenID& token_id) const;

    // 机器人 发送心跳
    int SendGamePulse();

private:
    //辅助函数 WithLock 标识调用前需要获得此对象的数据锁mutex

    int GetRobotWithLock(const UserID& userid, RobotPtr& robot) const;

    int SetRobotWithLock(const RobotPtr& robot);

    int GetRobotByTokenWithLock(const TokenID& token_id, RobotPtr& robot) const;

public:
    // 对象状态快照
    int SnapShotObjectStatus();

private:
    int CheckNotInnerThread();

private:
    //机器人 数据锁
    mutable std::mutex robot_map_mutex_;

    //机器人 游戏服务器连接集合 (只增不减)
    RobotMap robot_map_;

    //机器人 接收消息线程
    YQThread notify_thread_;

    //机器人 心跳线程
    YQThread heart_thread_;

};

#define RobotMgr RobotNetManager::Instance()