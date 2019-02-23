#pragma once
#include "RobotDef.h"
#include "Robot.h"

// �����˹�����
class CRobotMgr : public ISingletion<CRobotMgr> {
public:
    using DepositMap = std::unordered_map<UserID, DepositType>;
    using RobotMap = std::unordered_map<UserID, RobotPtr>;
    using HallLogonMap = std::unordered_map<UserID, HallLogonStatusType>;
    using HallRoomDataMap = std::unordered_map<int32_t, HallRoomData>;

public:
    int Init();

    void Term();

protected:
    SINGLETION_CONSTRUCTOR(CRobotMgr);

private:
    // ���� ��������
    int ConnectHall(bool bReconn = false);

    // ���� ��Ϣ����
    int SendHallRequestWithLock(RequestID nReqId, uint32_t& nDataLen, void *pData, RequestID &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho = true, uint32_t wait_ms = REQ_TIMEOUT_INTERVAL);

    // ���� ��Ϣ����
    void ThreadHallNotify();

    // ���� ��Ϣ����
    void OnHallNotify(RequestID nReqId, void* pDataPtr, int32_t nSize);

    // ���� �Ͽ�����
    void OnDisconnHall(RequestID nReqId, void* pDataPtr, int32_t nSize);

    // ���� ��ʱ����
    void ThreadHallPluse();

    // ������ ��ʱ����
    void ThreadRobotPluse();

    // ������ ��Ϣ����
    void ThreadRobotNotify();

    // ������ ��Ϣ����
    void OnRobotNotify(RequestID nReqId, void* pDataPtr, int32_t nSize, TokenID nTokenID);

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
    int RobotGainDeposit(UserID userid, int32_t amount);
    int RobotBackDeposit(UserID userid, int32_t amount);

    // ������
    RobotPtr GetRobotWithLock(UserID userid);
    void SetRobotWithLock(RobotPtr client);
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
    UThread	hall_notify_thread_;
    //���� �����߳�
    UThread	hall_heart_timer_thread_;

    //������ ������
    std::mutex robot_map_mutex_;
    //������ ��Ϸ���������Ӽ���
    RobotMap robot_map_;
    //������ ������Ϣ�߳�
    UThread	robot_notify_thread_;
    //������ �����߳�
    UThread	robot_heart_timer_thread_;

    //��̨���� ������
    std::mutex deposit_map_mutex_;
    //��̨���� ����
    DepositMap deposit_map_;
    //��̨���� ��ʱ���߳�
    UThread	deposit_timer_thread_;

    //������ �߳�
    UThread	main_timer_thread_;

};
#define TheRobotMgr CRobotMgr::Instance()