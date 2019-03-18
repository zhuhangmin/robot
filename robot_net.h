#pragma once
#include "robot_define.h"

class RobotNet {
public:
    explicit RobotNet(const UserID& userid);

public:
    // 游戏 连接
    int Connect(const std::string& game_ip, const int& game_port, const ThreadID& game_notify_thread_id);

    // 游戏 断开
    int OnDisconnect();

    // 游戏 连接状态
    int IsConnected(BOOL& is_connected) const;

    // 进入游戏
    int SendEnterGame(const RoomID& roomid, const TableNO& tableno);

    // 发送心跳
    int SendPulse();

    // 连接保活
    int KeepConnection();

public:
    // 属性接口
    UserID GetUserID() const;

    int GetTokenID(TokenID& token) const; //允许脏读

    TimeStamp GetTimeStamp() const;

    void SetTimeStamp(const TimeStamp& val);

private:
    // 辅助函数 WithLock 标识调用前需要获得此对象的数据锁mutex
    int SendGameRequestWithLock(const RequestID& requestid, const google::protobuf::Message &val, REQUEST& response, const bool& need_echo = true) const;

    int ResetDataWithLock();

    int InitDataWithLock();

public:
    // 对象状态快照
    int SnapShotObjectStatus() const;

private:
    // 配置机器人ID 初始化后不在改变,不需要锁保护
    UserID userid_{0};

    // 数据锁
    mutable std::mutex mutex_;

    // 机器人游戏服务器连接
    CDefSocketClientPtr connection_;

    // 游戏服 IP
    std::string game_ip_;

    // 游戏服 PORT
    int game_port_{InvalidPort};;

    // Manager中所有机器人通知消息处理线程id
    ThreadID game_notify_thread_id_{0};

    // 心跳超时计数
    int timeout_count_{0};

    // 心跳时间戳
    TimeStamp timestamp_{std::time(nullptr)};

    // 重连标签
    bool need_reconnect_{false};

};

using RobotPtr = std::shared_ptr<RobotNet>;