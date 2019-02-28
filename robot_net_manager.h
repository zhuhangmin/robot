#pragma once
#include "robot_define.h"
#include "robot_net.h"

// 机器人管理器
class RobotGameManager : public ISingletion<RobotGameManager> {
public:
    using RobotMap = std::unordered_map<UserID, RobotPtr>;
public:
    int Init();

    int Term();

public:
    // 获得（不存在创建）指定机器人
    int GetRobotWithCreate(const UserID userid, RobotPtr& robot);

    // 获得所有机器人接收消息的线程
    ThreadID GetRobotNotifyThreadID();

protected:
    SINGLETION_CONSTRUCTOR(RobotGameManager);

private:
    // 机器人 定时心跳
    int ThreadRobotPluse();

    // 机器人 消息接收
    int ThreadRobotNotify();

    // 机器人 消息处理
    int OnRobotNotify(const RequestID requestid, void* ntf_data_ptr, const int data_size, const TokenID token_id);

    // 机器人 发送心跳
    int SendGamePluse();

private:
    //辅助函数 WithLock 标识调用前需要获得此对象的数据锁mutex

    int GetRobotWithLock(const UserID userid, RobotPtr& robot) const;

    int SetRobotWithLock(RobotPtr robot);

    int GetRobotByTokenWithLock(const TokenID token_id, RobotPtr& robot) const;

public:
    // 对象状态快照
    int SnapShotObjectStatus();

private:
    //机器人 数据锁
    mutable std::mutex robot_map_mutex_;

    //机器人 游戏服务器连接集合
    RobotMap robot_map_;

    //机器人 接收消息线程
    YQThread robot_notify_thread_;

    //机器人 心跳线程
    YQThread robot_heart_timer_thread_;

};

#define RobotMgr RobotGameManager::Instance()