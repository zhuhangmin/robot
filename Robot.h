#pragma once
#include "RobotDef.h"



class Robot {
public:
    Robot();
    Robot(UserID userid);

public:
    // ��Ϸ ����
    int ConnectGame(const std::string& game_ip, const int game_port, ThreadID game_notify_thread_id);

    // ��Ϸ �Ͽ�
    void DisconnectGame();

    // ��Ϸ ����״̬
    void IsConnected();

public:
    // ����ҵ��

    // ������Ϸ
    int SendEnterGame(RoomID roomid);

    // ��������
    void SendGamePulse();

public:
    // ���Խӿ�
    UserID GetUserID();

    TokenID	GetTokenID();

private:
    // ��������
    int SendGameRequestWithLock(RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool need_echo = true);

private:
    // ���û�����ID ��ʼ�����ڸı�,����Ҫ������
    UserID userid_{0};

    // ��Ϸ����
    std::mutex mutex_;
    CDefSocketClientPtr game_connection_{std::make_shared<CDefSocketClient>()};

};

using RobotPtr = std::shared_ptr<Robot>;