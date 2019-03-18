#pragma once
#include "robot_define.h"

class RobotHallManager : public ISingletion<RobotHallManager> {
public:
    // ��WithLock��׺��������������ж��߳̿ɼ����� һ�㶼��Ҫ����������ҵ�����������
    // ��� lock + WithLock ������϶���, ��֤�����ö��mutex��std::mutex Ϊ�ǵݹ���
    // ֻ���ⲿ�߳̿ɼ�
    int Init();

    int Term();

    // ���� ��½
    int LogonHall(const UserID& userid);

    // ���� ����û���Ϣ
    int GetUserGameInfo(const UserID& userid);

    // ���� ���õ�½״̬
    int SetLogonStatus(const UserID& userid, const HallLogonStatusType& status);

    // ���� ��������״̬
    int SetDepositUpdate(const UserID& userid);

    // ��ô����������� ���� hall_room_data COPY
    int GetHallRoomData(const RoomID& roomid, HallRoomData& hall_room_data);

    // ���ѡ��û�е�½�����Ļ�����
    int GetRandomNotLogonUserID(UserID& random_userid);

private:
    // ���� ��ʱ��Ϣ
    int ThreadTimer();

    // ���� ���»�����������Ϣ
    int SendUpdateUserDeposit();

    // ���� ��Ϣ����
    int ThreadNotify();

    // ���� ��Ϣ����
    int OnHallNotify(const RequestID& requestid, void* ntf_data_ptr, const int& data_size);

    // ���� �Ͽ�����
    int OnDisconnect();

    // ���� ����
    int SendPulse();

    // ���� ���з�������
    int SendGetAllRoomData();

private:
    // �������� WithLock ��ʶ����ǰ��Ҫ��ô˶����������mutex

    // ���� ��������
    int ConnectWithLock() const;

    // ���� ��Ϣ����
    int SendRequestWithLock(const RequestID& requestid, int& data_size, void *req_data_ptr, RequestID &response_id, std::shared_ptr<void> &resp_data_ptr, const bool& need_echo = true) const;

    // ���� ��õ�½״̬
    int GetLogonStatusWithLock(const UserID& userid, HallLogonStatusType& status) const;

    int SetLogonStatusWithLock(const UserID& userid, const HallLogonStatusType& status);

    // ���� ��÷�������
    int GetHallRoomDataWithLock(const RoomID& roomid, HallRoomData& hall_room_data) const;

    // ���� ���÷�������
    int SetHallRoomDataWithLock(const RoomID& roomid, HallRoomData* hall_room_data);

    // ���� ��ʼ�����Ӻ�����
    int InitDataWithLock();

    // ���� �������Ӻ�����
    int ResetDataWithLock();

    // ���� ���з�������
    int SendGetAllRoomDataWithLock();

    // ���� ��������
    int SendGetRoomDataWithLock(const RoomID& roomid);

    // ���� ����û���Ϣ
    int GetUserGameInfoWithLock(const UserID& userid);

    // ���ӱ���
    int KeepConnection();

public:
    // ��ҵ�� ���Ժ���
    // ����״̬����
    int SnapShotObjectStatus();

    int BriefInfo() const;

private:
    // �������߳̿ɼ��Լ��
    int CheckNotInnerThread();

protected:
    SINGLETION_CONSTRUCTOR(RobotHallManager);
private:
    // ����һ������һ������Ӧ��ϵ,��ֹ�����Ƕ���������
    // ���ֱ����ж����ʱ˵���������ݾۺ�̫�࣬��Ҫ�ֳɶ������

    //���� ������
    mutable std::mutex mutex_;

    //���� ����
    CDefSocketClientPtr connection_{std::make_shared<CDefSocketClient>()};

    //���� ��½״̬
    HallLogonMap logon_status_map_;

    //���� ��������
    HallRoomDataMap room_data_map_;

    //���� ������Ϣ�߳�
    YQThread	notify_thread_;

    //���� �����߳�
    YQThread	timer_thread_;

    //���� ��ʱ����
    int timeout_count_{0};

    //���� �Ƿ���Ҫ������ǩ
    bool need_reconnect_{false};

    //���� ���»��������� ����
    RobotUserIDMap update_depost_map_;

    //���� ����ʱ���
    TimeStamp timestamp_{std::time(nullptr)};

    //���� ������Ϣͬ��ʱ���
    TimeStamp room_data_timestamp_{std::time(nullptr)};

};

#define HallMgr  RobotHallManager::Instance()