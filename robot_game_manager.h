#pragma once
#include "robot_define.h"
#include "robot_game.h"

// 机器人管理器
class RobotGameManager : public ISingletion<RobotGameManager> {
public:
    using RobotMap = std::unordered_map<UserID, RobotPtr>;

public:
    int Init();

    void Term();

public:
    RobotPtr GetRobotWithCreate(UserID userid);

    ThreadID GetRobotNotifyThreadID();
protected:
    SINGLETION_CONSTRUCTOR(RobotGameManager);

private:
    // 机器人 定时心跳
    void ThreadRobotPluse();

    // 机器人 消息接收
    void ThreadRobotNotify();

    // 机器人 消息处理
    int OnRobotNotify(RequestID requestid, void* ntf_data_ptr, int data_size, TokenID token_id);

private:
    //具体业务

    // 游戏 心跳
    void SendGamePluse();

    // 机器人
    RobotPtr GetRobotWithLock(UserID userid);
    void SetRobotWithLock(RobotPtr robot);
    int GetRobotByTokenWithLock(const TokenID token_id, RobotPtr& robot);

private:
    //机器人 数据锁
    std::mutex robot_map_mutex_;

    //机器人 游戏服务器连接集合
    RobotMap robot_map_;

    //机器人 接收消息线程
    YQThread robot_notify_thread_;

    //机器人 心跳线程
    YQThread robot_heart_timer_thread_;

};

#define RobotMgr RobotGameManager::Instance()