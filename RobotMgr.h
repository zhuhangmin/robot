#pragma once
#include "RobotDef.h"
#include "Robot.h"

// �����˹�����
class CRobotMgr : public ISingletion<CRobotMgr> {
public:
    using RobotMap = std::unordered_map<UserID, RobotPtr>;

public:
    bool Init();

    void Term();

protected:
    SINGLETION_CONSTRUCTOR(CRobotMgr);

private:
    // ������������
    bool ConnectHall(bool bReconn = false);

    // ����������Ϣ
    void ThreadHallNotify();

    // ��������������
    int SendHallRequest(RequestID nReqId, uint32_t& nDataLen, void *pData, RequestID &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho = true, uint32_t wait_ms = REQ_TIMEOUT_INTERVAL);

    // ������Ϣ����
    void OnHallNotify(RequestID nReqId, void* pDataPtr, int32_t nSize);

    // �����Ͽ�����
    void OnDisconnHallWithLock(RequestID nReqId, void* pDataPtr, int32_t nSize);

    // ��ʱ�� ��������
    void ThreadSendPluse();

    // ��ʱ�� ������ҵ��
    void ThreadMainProc();

    // ��ʱ�� ����
    void ThreadDeposit();

private:
    //����ҵ��

    // ��ʱ�� ��½����
    bool OnTimerLogonHall(time_t nCurrTime);

    // ������½���縨������
    int RobotLogonHall(const int32_t& account = 0);

    //@zhuhangmin 20190218 �������߳̿ɼ�
    void    OnTimerSendPluse(time_t nCurrTime);
    void    SendHallPluse(time_t nCurrTime);
    void    SendGamePluse(time_t nCurrTime);

    //@zhuhangmin 20190218 �������߳̿ɼ�
    void OnTimerUpdateDeposit(time_t nCurrTime);
    int RobotGainDeposit(RobotPtr client);
    int RobotBackDeposit(RobotPtr client);

    // ������
    RobotPtr GetRobotByToken(const EConnType& type, const TokenID& id);
    bool IsLogon(UserID userid);
    void SetLogon(UserID userid, bool status);
    RobotPtr GetRobotClient(UserID userid);
    void SetRobotClient(RobotPtr client);

    void SetRoomID(UserID userid, RoomID roomid);

    int GetRoomCurrentRobotSize(RoomID roomid); //������Ļ�������


private:
    UThread	main_timer_thread_;

    UThread	heart_timer_thread_;

    UThread	deposit_timer_thread_;

    UThread	hall_notify_thread_;

    std::mutex robot_map_mutex_;
    RobotMap robot_map_; //��������robot��̬��Ϣ

    std::mutex hall_connection_mutex_;
    CDefSocketClientPtr hall_connection_{std::make_shared<CDefSocketClient>()};//��������




};
#define TheRobotMgr CRobotMgr::Instance()