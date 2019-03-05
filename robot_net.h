#pragma once
#include "robot_define.h"

class RobotNet {
public:
    RobotNet(UserID userid);

public:
    // 游戏 连接
    int ConnectGame(const std::string& game_ip, const int& game_port, const ThreadID& game_notify_thread_id);

    // 游戏 断开
    int OnDisconnGame();

    // 游戏 连接状态
    BOOL IsConnected();

    // 进入游戏
    int SendEnterGame(const RoomID roomid);

    // 发送心跳
    int SendGamePulse();

public:
    // 属性接口
    UserID GetUserID() const;

    TokenID GetTokenID();

    TimeStamp GetTimeStamp() const;

    void SetTimeStamp(TimeStamp val);

public:
    // 对象状态快照
    int SnapShotObjectStatus();

private:
    // 辅助函数 WithLock 标识调用前需要获得此对象的数据锁mutex
    int SendGameRequestWithLock(const RequestID requestid, const google::protobuf::Message &val, REQUEST& response, const bool need_echo = true);

    int ResetDataWithLock();

    int InitDataWithLock();

    // ResetDataWithLock + InitDataWithLock
    int ResetInitDataWithLock();



private:
    // 配置机器人ID 初始化后不在改变,不需要锁保护
    UserID userid_{0};

    // 游戏连接
    mutable std::mutex mutex_;
    CDefSocketClientPtr game_connection_;
    std::string game_ip_;
    int game_port_{InvalidPort};;
    ThreadID game_notify_thread_id_{0};
    int pulse_timeout_count_{0};
    TimeStamp timestamp_{0};

};

using RobotPtr = std::shared_ptr<RobotNet>;