#pragma once
#include "robot_define.h"
#include "robot_net.h"

using RobotMap = std::unordered_map<UserID, RobotPtr>;

class RobotNetManager : public ISingletion<RobotNetManager> {
public:
    int Init();

    int Term();

    // 机器人 连接 发送进入游戏
    int EnterGame(const UserID& userid, const RoomID& roomid, const TableNO& tableno, const std::string& game_ip, const int& game_port);

private:
    // 机器人 定时消息
    int ThreadTimer();

    // 机器人 消息接收
    int ThreadNotify();

    // 机器人 消息处理
    int OnNotify(const RequestID& requestid, const REQUEST& request, const TokenID& token_id);

    // 机器人 连接保活
    int KeepConnection();

    // 单人结算
    int ResultOneUserWithLock(const UserID& robot_userid, const REQUEST &request) const;

    // 正桌结算
    int ResultTableWithLock(const UserID& robot_userid, const REQUEST &request) const;

private:
    //辅助函数 WithLock 标识调用前需要获得此对象的数据锁mutex

    int GetRobotWithLock(const UserID& userid, RobotPtr& robot) const;

    int SetRobotWithLock(const RobotPtr& robot);

    int GetRobotByTokenWithLock(const TokenID& token_id, RobotPtr& robot) const;

public:
    // 对象状态快照
    int SnapShotObjectStatus();

    int BriefInfo() const;

protected:
    SINGLETION_CONSTRUCTOR(RobotNetManager);

private:
    //机器人 数据锁
    mutable std::mutex mutex_;

    //机器人 游戏服务器连接集合 (只增不减)
    RobotMap robot_map_;

    //机器人 接收消息线程
    YQThread notify_thread_;

    //机器人 心跳线程
    YQThread timer_thread_;

};

#define RobotMgr RobotNetManager::Instance()