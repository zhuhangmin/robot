#pragma once
#include "robot_define.h"

class RobotNet {
public:
    explicit RobotNet(const UserID& userid);

public:
    // ��Ϸ ����
    int Connect(const std::string& game_ip, const int& game_port, const ThreadID& game_notify_thread_id);

    // ��Ϸ �Ͽ�
    int OnDisconnect();

    // ��Ϸ ����״̬
    int IsConnected(BOOL& is_connected) const;

    // ������Ϸ
    int SendEnterGame(const RoomID& roomid, const TableNO& tableno);

    // ��������
    int SendPulse();

    // ���ӱ���
    int KeepConnection();

public:
    // ���Խӿ�
    UserID GetUserID() const;

    int GetTokenID(TokenID& token) const; //�������

    TimeStamp GetTimeStamp() const;

    void SetTimeStamp(const TimeStamp& val);

private:
    // �������� WithLock ��ʶ����ǰ��Ҫ��ô˶����������mutex
    int SendGameRequestWithLock(const RequestID& requestid, const google::protobuf::Message &val, REQUEST& response, const bool& need_echo = true) const;

    int ResetDataWithLock();

    int InitDataWithLock();

public:
    // ����״̬����
    int SnapShotObjectStatus() const;

private:
    // ���û�����ID ��ʼ�����ڸı�,����Ҫ������
    UserID userid_{0};

    // ������
    mutable std::mutex mutex_;

    // ��������Ϸ����������
    CDefSocketClientPtr connection_;

    // ��Ϸ�� IP
    std::string game_ip_;

    // ��Ϸ�� PORT
    int game_port_{InvalidPort};;

    // Manager�����л�����֪ͨ��Ϣ�����߳�id
    ThreadID game_notify_thread_id_{0};

    // ������ʱ����
    int timeout_count_{0};

    // ����ʱ���
    TimeStamp timestamp_{std::time(nullptr)};

    // ������ǩ
    bool need_reconnect_{false};

};

using RobotPtr = std::shared_ptr<RobotNet>;