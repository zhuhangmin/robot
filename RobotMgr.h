#pragma once
#include "RobotDef.h"
#include "Robot.h"

// �����˹�����
class CRobotMgr : public ISingletion<CRobotMgr> {
public:
    using RobotMap = std::unordered_map<UserID, RobotPtr>;
    using DepositMap = std::unordered_map<UserID, DepositType>;

public:
    int Init();

    void Term();

protected:
    SINGLETION_CONSTRUCTOR(CRobotMgr);

private:
    // ������������
    int ConnectHall(bool bReconn = false);

    // ��������������
    int SendHallRequest(RequestID nReqId, uint32_t& nDataLen, void *pData, RequestID &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho = true, uint32_t wait_ms = REQ_TIMEOUT_INTERVAL);

    // ����������Ϣ
    void ThreadHallNotify();

    // ������Ϣ����
    void OnHallNotify(RequestID nReqId, void* pDataPtr, int32_t nSize);

    // �����Ͽ�����
    void OnDisconnHallWithLock(RequestID nReqId, void* pDataPtr, int32_t nSize);

    // ��ʱ�� ��������
    void ThreadSendHallPluse();

    // ��ʱ�� ������ҵ��
    void ThreadMainProc();

    // ��ʱ�� ����
    void ThreadDeposit();

private:
    //����ҵ��

    // ������½���縨������
    int LogonHall();

    //@zhuhangmin 20190218 �������߳̿ɼ�
    void    OnTimerSendPluse(time_t nCurrTime);
    void    SendHallPluse(time_t nCurrTime);
    void    SendGamePluse(time_t nCurrTime);

    //@zhuhangmin 20190218 �������߳̿ɼ�
    int RobotGainDeposit(UserID userid, int32_t amount);
    int RobotBackDeposit(UserID userid, int32_t amount);

    // ������
    RobotPtr GetRobotByToken(const EConnType& type, const TokenID& id);
    bool IsLogon(UserID userid);
    void SetLogon(UserID userid, bool status);
    RobotPtr GetRobot(UserID userid);
    void SetRobot(RobotPtr client);

    void SetRoomID(UserID userid, RoomID roomid);

    int GetRoomCurrentRobotSize(RoomID roomid); //������Ļ�������

private:
    //������ ��ʱ���߳�
    UThread	main_timer_thread_;

    //���� ��ʱ���߳�
    UThread	heart_timer_thread_;

    //���� ��ʱ���߳�
    UThread	deposit_timer_thread_;

    //���� ������Ϣ�߳�
    UThread	hall_notify_thread_;

    //���л����˶�̬��Ϣ
    std::mutex robot_map_mutex_;
    RobotMap robot_map_;

    //��������
    std::mutex hall_connection_mutex_;
    CDefSocketClientPtr hall_connection_{std::make_shared<CDefSocketClient>()};

    //��������
    std::mutex deposit_map_mutex_;
    DepositMap deposit_map_;


};
#define TheRobotMgr CRobotMgr::Instance()