#pragma once
#include "robot_define.h"
class RobotHallManager : public ISingletion<RobotHallManager> {
public:

public:
    int Init();

    int Term();

public:
    // ���� ҵ��

    // ���� ��½
    int LogonHall(const UserID userid);

    // ���� ����
    int SendHallPluse();

    // ���� ��������
    int SendGetRoomData(const RoomID roomid);

    // ���� ���з�������
    int SendGetAllRoomData();

public:
    // ��ô�����������
    int GetHallRoomData(const RoomID& roomid, HallRoomData& hall_room_data);

    // ���ѡ��û�е�½�����Ļ�����
    int GetRandomNotLogonUserID(UserID& random_userid) const;

protected:
    SINGLETION_CONSTRUCTOR(RobotHallManager);

private:
    // ���� ��������
    int ConnectHall();

    // ���� ��Ϣ����
    int SendHallRequestWithLock(const RequestID requestid, int& data_size, void *req_data_ptr, RequestID &response_id, std::shared_ptr<void> &resp_data_ptr, bool need_echo = true);

    // ���� ��Ϣ����
    int ThreadHallNotify();

    // ���� ��Ϣ����
    int OnHallNotify(const RequestID requestid, void* ntf_data_ptr, const int data_size);

    // ���� �Ͽ�����
    int OnDisconnHall(const RequestID requestid, void* data_ptr, const int data_size);

    // ���� ��ʱ����
    int ThreadHallPluse();

private:
    //�������� WithLock ��ʶ����ǰ��Ҫ��ô˶����������mutex

    int GetLogonStatusWithLock(const UserID& userid, HallLogonStatusType& status) const;

    int SetLogonStatusWithLock(const UserID userid, HallLogonStatusType status);

    int GetHallRoomDataWithLock(const RoomID& roomid, HallRoomData& hall_room_data) const;

    int SetHallRoomDataWithLock(const RoomID roomid, HallRoomData* hall_room_data);

private:
    //���� ������
    mutable std::mutex hall_connection_mutex_;
    //���� ����
    CDefSocketClientPtr hall_connection_{std::make_shared<CDefSocketClient>()};
    //���� ��½״̬
    HallLogonMap hall_logon_status_map_;
    //���� ��������
    HallRoomDataMap hall_room_data_map_;
    //���� ������Ϣ�߳�
    YQThread	hall_notify_thread_;
    //���� �����߳�
    YQThread	hall_heart_timer_thread_;
};

#define HallMgr  RobotHallManager::Instance()