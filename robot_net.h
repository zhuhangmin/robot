#pragma once
#include "robot_define.h"

class RobotNet {
public:
    RobotNet(UserID userid);

public:
    // ��Ϸ ����
    int ConnectGame(const std::string& game_ip, const int& game_port, const ThreadID& game_notify_thread_id);

    // ��Ϸ �Ͽ�
    int OnDisconnGame();

    // ��Ϸ ����״̬
    BOOL IsConnected();

    // ������Ϸ
    int SendEnterGame(const RoomID roomid);

    // ��������
    int SendGamePulse();

public:
    // ���Խӿ�
    UserID GetUserID() const;

    TokenID GetTokenID();

    TimeStamp GetTimeStamp() const;

    void SetTimeStamp(TimeStamp val);

public:
    // ����״̬����
    int SnapShotObjectStatus();

private:
    // �������� WithLock ��ʶ����ǰ��Ҫ��ô˶����������mutex
    int SendGameRequestWithLock(const RequestID requestid, const google::protobuf::Message &val, REQUEST& response, const bool need_echo = true);

    int ResetDataWithLock();

    int InitDataWithLock();

    // ResetDataWithLock + InitDataWithLock
    int ResetInitDataWithLock();



private:
    // ���û�����ID ��ʼ�����ڸı�,����Ҫ������
    UserID userid_{0};

    // ��Ϸ����
    mutable std::mutex mutex_;
    CDefSocketClientPtr game_connection_;
    std::string game_ip_;
    int game_port_{InvalidPort};;
    ThreadID game_notify_thread_id_{0};
    int pulse_timeout_count_{0};
    TimeStamp timestamp_{0};

};

using RobotPtr = std::shared_ptr<RobotNet>;