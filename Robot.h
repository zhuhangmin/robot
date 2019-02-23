#pragma once
#include "RobotDef.h"



class Robot {
public:
    Robot();
    Robot(UserID userid);

public:
    // 游戏 连接
    int ConnectGame(const std::string& game_ip, const int game_port, ThreadID game_notify_thread_id);

    // 游戏 断开
    void DisconnectGame();

    // 游戏 连接状态
    void IsConnected();

public:
    // 具体业务

    // 进入游戏
    int SendEnterGame(RoomID roomid);

    // 发送心跳
    void SendGamePulse();

public:
    // 属性接口
    UserID GetUserID();

    TokenID	GetTokenID();

private:
    // 辅助函数
    int SendGameRequestWithLock(RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool need_echo = true);

private:
    // 配置机器人ID 初始化后不在改变,不需要锁保护
    UserID userid_{0};

    // 游戏连接
    std::mutex mutex_;
    CDefSocketClientPtr game_connection_{std::make_shared<CDefSocketClient>()};

};

using RobotPtr = std::shared_ptr<Robot>;