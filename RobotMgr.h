#pragma once
#include "RobotDef.h"
#include "Robot.h"

// �����˹�����
class CRobotMgr : public ISingletion<CRobotMgr> {
public:
    using DepositMap = std::unordered_map<UserID, DepositType>;
    using RobotMap = std::unordered_map<UserID, RobotPtr>;
    using HallLogonStatusMap = std::unordered_map<UserID, HallLogonStatusType>;


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

    // ��ʱ ҵ��������
    void ThreadMainProc();

    // ��ʱ ��������
    void ThreadDeposit();

private:
    //����ҵ��

    // ������½���縨������
    int LogonHall(UserID userid);

    //@zhuhangmin 20190218 �������߳̿ɼ�
    void    OnTimerSendPluse(time_t nCurrTime);
    void    SendHallPluse();
    void    SendGamePluse();

    //@zhuhangmin 20190218 �������߳̿ɼ�
    int RobotGainDeposit(UserID userid, int32_t amount);
    int RobotBackDeposit(UserID userid, int32_t amount);

    // ������
    RobotPtr GetRobotWithLock(UserID userid);

    void SetRobotWithLock(RobotPtr client);

    RobotPtr GetRobotByToken(const EConnType& type, const TokenID& id);

private:
    //guard by lock
    int GetLogonStatusWithLock(const UserID& userid, HallLogonStatusType& status);
    void SetLogonStatusWithLock(const UserID userid, HallLogonStatusType status);

    int GetDepositTypeWithLock(const UserID& userid, DepositType& type);
    void SetDepositTypesWithLock(const UserID userid, DepositType type);

private:
    int GetRandomNotLogonUserID(UserID& random_userid);

private:
    // ����ҵ�񻮷����ݺ��� 

    //���� ������
    std::mutex hall_connection_mutex_;
    //���� ����
    CDefSocketClientPtr hall_connection_{std::make_shared<CDefSocketClient>()};
    //���� ��½״̬
    HallLogonStatusMap hall_logon_status_map_;
    //���� ������Ϣ�߳�
    UThread	hall_notify_thread_;
    //���� �����߳�
    UThread	hall_heart_timer_thread_;

    //������ ������
    std::mutex robot_map_mutex_;
    //������ ��Ϸ���������Ӽ���
    RobotMap robot_map_;
    //������ �����߳�
    UThread	robot_heart_timer_thread_;

    //���� ������
    std::mutex deposit_map_mutex_;
    //���� ����
    DepositMap deposit_map_;
    //���� ��ʱ���߳�
    UThread	deposit_timer_thread_;


    //������ ��ʱ���߳�
    UThread	main_timer_thread_;

};
#define TheRobotMgr CRobotMgr::Instance()