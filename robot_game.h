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
    BOOL IsConnected();

public:
    // ����ҵ��

    // ������Ϸ
    int SendEnterGame(const RoomID roomid);

    // ��������
    int SendGamePulse();

public:
    // ���Խӿ�
    UserID GetUserID() const;

    TokenID GetTokenID();

public:
    // ����״̬����
    int SnapShotObjectStatus();

private:
    // �������� WithLock ��ʶ����ǰ��Ҫ��ô˶����������mutex
    int SendGameRequestWithLock(const RequestID requestid, const google::protobuf::Message &val, REQUEST& response, const bool need_echo = true);

private:
    // ���û�����ID ��ʼ�����ڸı�,����Ҫ������
    UserID userid_{0};

    // ��Ϸ����
    std::mutex mutex_;
    CDefSocketClientPtr game_connection_;

};

using RobotPtr = std::shared_ptr<Robot>;