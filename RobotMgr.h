#pragma once
#include "RobotDef.h"
#include "Robot.h"

// �����˹�����
class CRobotMgr : public ISingletion<CRobotMgr> {
public:
    //robot
    using RobotMap = std::unordered_map<UserID, RobotPtr>;

public:
    // ��ʼ|����
    bool Init();
    void Term();



protected:
    // ��������������
    TTueRet SendHallRequest(RequestID nReqId, uint32_t& nDataLen, void *pData, RequestID &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho = true, uint32_t wait_ms = REQ_TIMEOUT_INTERVAL);

    void TimerThreadProc();

    // ����ʱ��

    void OnThreadTimer(std::time_t nCurrTime);

    bool	InitConnectHall(bool bReconn = false);

    bool	InitNotifyThreads();

    RobotPtr GetRobotByToken(const EConnType& type, const TokenID& id);

    // ֪ͨ��Ϣ�̷߳���
    void	ThreadRunHallNotify();

    // ֪ͨ��Ϣ����ص�
    void OnHallNotify(RequestID nReqId, void* pDataPtr, int32_t nSize);

    // ��ʱ���ص�����
    bool	OnTimerLogonHall(time_t nCurrTime);
    /*   void    OnTimerSendHallPluse(time_t nCurrTime);
       void    OnTimerSendGamePluse(time_t nCurrTime);*/

    //@zhuhangmin 20190218 �������߳̿ɼ� beg
    void    OnTimerUpdateDeposit(time_t nCurrTime);
    TTueRet RobotGainDeposit(RobotPtr client);
    TTueRet RobotBackDeposit(RobotPtr client);
    //@zhuhangmin 20190218 �������߳̿ɼ� end

    // ���縨������
    TTueRet	RobotLogonHall(const int32_t& account = 0);

    //@zhuhangmin
    bool IsLogon(UserID userid);
    void SetLogon(UserID userid, bool status);
    RobotPtr GetRobotClient(UserID userid);
    void SetRobotClient(RobotPtr client);

    void SetRoomID(UserID userid, RoomID roomid);

    int GetRoomCurrentRobotSize(RoomID roomid); //��ǰ��ĳ��������Ļ�������

private:
    void OnDisconnHallWithLock(RequestID nReqId, void* pDataPtr, int32_t nSize);


protected:
    SINGLETION_CONSTRUCTOR(CRobotMgr);

    UThread				g_thrdTimer;

    UThread			m_thrdHallNotify;

    // mutable������ �����
    std::mutex        robot_map_mutex_;
    RobotMap robot_map_; //��������robot��̬��Ϣ

    std::mutex hall_connection_mutex_;
    CDefSocketClientPtr hall_connection_{std::make_shared<CDefSocketClient>()};//��������


};
#define TheRobotMgr CRobotMgr::Instance()