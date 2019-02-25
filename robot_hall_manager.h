#pragma once
#include "robot_define.h"
class RobotHallManager : public ISingletion<RobotHallManager> {
public:
    using HallLogonMap = std::unordered_map<UserID, HallLogonStatusType>;
    using HallRoomDataMap = std::unordered_map<RoomID, HallRoomData>;
public:
    int Init();

    void Term();

public:
    // ���� ҵ��

    // ���� ��½
    int LogonHall(UserID userid);

    // ���� ����
    void SendHallPluse();

    // ���� ��������
    int SendGetRoomData(RoomID roomid);

    // ���� ���з�������
    int SendGetAllRoomData();

public:
    // ��ô�����������
    int GetHallRoomData(const RoomID& roomid, HallRoomData& hall_room_data);

    // ���ѡ��û�е�½�����Ļ�����
    int GetRandomNotLogonUserID(UserID& random_userid);

protected:
    SINGLETION_CONSTRUCTOR(RobotHallManager);

private:
    // ���� ��������
    int ConnectHall(bool bReconn = false);

    // ���� ��Ϣ����
    int SendHallRequestWithLock(RequestID requestid, int& data_size, void *req_data_ptr, RequestID &response_id, std::shared_ptr<void> &resp_data_ptr, bool need_echo = true);

    // ���� ��Ϣ����
    void ThreadHallNotify();

    // ���� ��Ϣ����
    void OnHallNotify(RequestID requestid, void* ntf_data_ptr, int data_size);

    // ���� �Ͽ�����
    void OnDisconnHall(RequestID requestid, void* data_ptr, int data_size);

    // ���� ��ʱ����
    void ThreadHallPluse();

private:
    //guard by lock

    int GetLogonStatusWithLock(const UserID& userid, HallLogonStatusType& status);

    int SetLogonStatusWithLock(const UserID userid, HallLogonStatusType status);

    int GetHallRoomDataWithLock(const RoomID& roomid, HallRoomData& hall_room_data);

    int SetHallRoomDataWithLock(const RoomID roomid, HallRoomData* hall_room_data);

private:
    //���� ������
    std::mutex hall_connection_mutex_;
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