#pragma once
#include "robot_define.h"
#include "robot_game.h"

// �����˹�����
class CRobotMgr : public ISingletion<CRobotMgr> {
public:
    using DepositMap = std::unordered_map<UserID, DepositType>;
    using RobotMap = std::unordered_map<UserID, RobotPtr>;
    using HallLogonMap = std::unordered_map<UserID, HallLogonStatusType>;
    using HallRoomDataMap = std::unordered_map<RoomID, HallRoomData>;

public:
    int Init();

    void Term();

protected:
    SINGLETION_CONSTRUCTOR(CRobotMgr);

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

    // ������ ��ʱ����
    void ThreadRobotPluse();

    // ������ ��Ϣ����
    void ThreadRobotNotify();

    // ������ ��Ϣ����
    void OnRobotNotify(RequestID requestid, void* ntf_data_ptr, int data_size, TokenID token_id);

    // �������� ��ʱ����
    void ThreadDeposit();

    // ҵ��������
    void ThreadMainProc();

private:
    //����ҵ��

    // ������½���縨������
    int LogonHall(UserID userid);
    void SendHallPluse();
    int SendGetRoomData(RoomID roomid);

    // game
    void    SendGamePluse();

    //@zhuhangmin 20190218 �������߳̿ɼ�
    int RobotGainDeposit(UserID userid, int amount);
    int RobotBackDeposit(UserID userid, int amount);

    // ������
    RobotPtr GetRobotWithLock(UserID userid);
    void SetRobotWithLock(RobotPtr robot);
    RobotPtr GetRobotByTokenWithLock(const TokenID& id);

private:
    //guard by lock
    int GetLogonStatusWithLock(const UserID& userid, HallLogonStatusType& status);
    void SetLogonStatusWithLock(const UserID userid, HallLogonStatusType status);

    int GetDepositTypeWithLock(const UserID& userid, DepositType& type);
    void SetDepositTypesWithLock(const UserID userid, DepositType type);

    int GetHallRoomData(const RoomID& roomid, HallRoomData& hall_room_data);
    int GetHallRoomDataWithLock(const RoomID& roomid, HallRoomData& hall_room_data);
    void SetHallRoomDataWithLock(const RoomID roomid, HallRoomData* hall_room_data);

private:
    // ���ѡ��û�е�½�����Ļ�����
    int GetRandomNotLogonUserID(UserID& random_userid);

private:
    // ��������״̬��Դ ���� ���ݺ�����Χ 
    // 1 ����, 2 ��Ϸ, 3 ��̨����

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

    //������ ������
    std::mutex robot_map_mutex_;
    //������ ��Ϸ���������Ӽ���
    RobotMap robot_map_;
    //������ ������Ϣ�߳�
    YQThread	robot_notify_thread_;
    //������ �����߳�
    YQThread	robot_heart_timer_thread_;

    //��̨���� ������
    std::mutex deposit_map_mutex_;
    //��̨���� ����
    DepositMap deposit_map_;
    //��̨���� ��ʱ���߳�
    YQThread	deposit_timer_thread_;

    //������ �߳�
    YQThread	main_timer_thread_;

};
