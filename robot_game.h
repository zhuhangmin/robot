#pragma once
#include "robot_define.h"



class Robot {
public:
    Robot(UserID userid);

public:
    // ��Ϸ ����
    int ConnectGame(const std::string& game_ip, const int game_port, const ThreadID game_notify_thread_id);

    // ��Ϸ �Ͽ�
    int DisconnectGame();

    // ��Ϸ ����״̬
    BOOL IsConnected() const;

public:
    // ����ҵ��

    // ������Ϸ
    int SendEnterGame(const RoomID roomid);

    // ��������
    int SendGamePulse();

public:
    // ���Խӿ�
    UserID GetUserID() const;

    TokenID GetTokenID() const;

private:
    // �������� WithLock ��ʶ����ǰ��Ҫ��ô˶����������mutex
    int SendGameRequestWithLock(const RequestID requestid, const google::protobuf::Message &val, REQUEST& response, const bool need_echo = true);

private:
    // ���û�����ID ��ʼ�����ڸı�,����Ҫ������
    UserID userid_{0};

    // ��Ϸ����
    mutable std::mutex mutex_;
    CDefSocketClientPtr game_connection_{std::make_shared<CDefSocketClient>()};

};

using RobotPtr = std::shared_ptr<Robot>;