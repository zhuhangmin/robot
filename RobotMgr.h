#pragma once
#include "RobotDef.h"
#include "Robot.h"

// �����˹�����
class CRobotMgr : public ISingletion<CRobotMgr> {
public:
    //setting
    using RoomSettingMap = std::unordered_map<RoomID, RoomSetiing>;
    using AccountSettingMap = std::unordered_map<AccountID, RobotSetting>;

    //game info
    using RoomMap = std::unordered_map<RoomID, game::base::RoomData>;

    //robot
    using RobotMap = std::unordered_map<UserID, RobotPtr>;
    using RoomCurUsersMap = std::unordered_map<RoomID, CurUserCount>;

public:
    // ��ʼ|����
    bool Init();
    void Term();

    // ��������������
    TTueRet SendHallRequest(RequestID nReqId, uint32_t& nDataLen, void *pData, RequestID &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho = true, uint32_t wait_ms = REQ_TIMEOUT_INTERVAL);

    // ���������ýӿ�
    bool GetRobotSetting(int account, RobotSetting& robot_setting_);

    //int32_t ApplyRobotForRoom(int32_t nGameId, int32_t nRoomId, int32_t nMaxCount); // second level
    //int32_t ApplyRobotForRoom(int32_t nGameId, int32_t nRoomId, TInt32Vec accounts); //should be first level

    // ���������ݹ���
    uint32_t	  GetAccountSettingSize() { return account_setting_map_.size(); }
    RobotPtr GetRobotByToken(const EConnType& type, const TokenID& id);


    // ����ʱ��
    void	OnServerMainTimer(time_t nCurrTime);

    //// RoomData
    //bool	GetRoomData(int32_t nRoomId, OUT ROOM& room);
    //uint32_t GetRoomDataLastTime(int32_t nRoomId);
    //void	SetRoomData(int32_t nRoomId, const LPROOM& room);

protected:
    // ��ʼ�������ļ�
    bool InitSetting();

    bool	InitNotifyThreads();

    bool	InitEnterGameThreads();

    bool	InitConnectHall(bool bReconn = false);

    int InitConnectGame();

    //bool	InitGameRoomDatas();

    //@zhuhangmin new pb
    int SendValidateReq();
    int SendGetGameInfo(RoomID roomid = 0);

    //@zhuhangmin new pb

    // ֪ͨ��Ϣ�̷߳���
    void	ThreadRunHallNotify();
    void	ThreadRunRoomNotify();
    void	ThreadRunGameNotify();
    void	ThreadRunGameInfoNotify();
    void	ThreadRunEnterGame();

    // ֪ͨ��Ϣ����ص�
    void OnHallNotify(RequestID nReqId, void* pDataPtr, int32_t nSize);
    void OnRoomNotify(RobotPtr client, RequestID nReqId, void* pDataPtr, int32_t nSize);
    void OnGameNotify(RobotPtr client, RequestID nReqId, void* pDataPtr, int32_t nSize);

    void OnRoomRobotEnter(RobotPtr client, int32_t nTableNo, int32_t nChairNo, std::string sEnterWay);


    // ��ʱ���ص�����
    bool	OnTimerLogonHall(time_t nCurrTime);
    void    OnTimerSendHallPluse(time_t nCurrTime);
    void    OnTimerSendRoomPluse(time_t nCurrTime);
    void    OnTimerSendGamePluse(time_t nCurrTime);
    void    OnTimerCtrlRoomActiv(time_t nCurrTime);

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

    bool IsInRoom(UserID userid);
    void SetRoomID(UserID userid, RoomID roomid);

    int GetRoomCurrentRobotSize(RoomID roomid); //��ǰ��ĳ��������Ļ�������

private:
    void OnDisconnHallWithLock(RequestID nReqId, void* pDataPtr, int32_t nSize);
    void OnDisconnRoomWithLock(RobotPtr client, RequestID nReqId, void* pDataPtr, int32_t nSize);
    void OnDisconnGameWithLock(RobotPtr client, RequestID nReqId, void* pDataPtr, int32_t nSize);
    void OnDisconnGameInfo();
    void OnCliDisconnGameInfoWithLock();
    void OnGameInfoNotify(RequestID nReqstID, const REQUEST &request);
    void OnRecvGameInfo(const REQUEST &request);
protected:
    SINGLETION_CONSTRUCTOR(CRobotMgr);

    UThread			m_thrdHallNotify;
    UThread			m_thrdRoomNotify;
    UThread			m_thrdGameNotify;
    UThread			m_thrdGameInfoNotify;

    UThread			m_thrdEnterGames[DEF_ENTER_GAME_THREAD_NUM];


    //immutable������ ���ã��ȸ���,�������),��ʼ����,���ı�״̬,���ü���
    AccountSettingMap	account_setting_map_; // �������˺�����  robot.setting
    RoomSettingMap room_setting_map_; //�����˷������� robot.setting
    //RoomDataMap	m_mapRoomData; // ������������ ������������� 

    //mutable������ �����
    std::mutex        robot_map_mutex_;
    RobotMap robot_map_; //��������robot��̬��Ϣ

    std::mutex hall_connection_mutex_;
    CDefSocketClientPtr hall_connection_{std::make_shared<CDefSocketClient>()};//��������

    std::mutex game_info_connection_mutex_; // ����ͬ����Ϸ������������Ϣ
    CDefSocketClientPtr game_info_connection_{std::make_shared<CDefSocketClient>()};//��Ϸ����
    RoomMap room_map_;



};
#define TheRobotMgr CRobotMgr::Instance()