#pragma once
#include "RobotDef.h"
#include "Robot.h"

// �����˹�����
class CRobotMgr : public ISingletion<CRobotMgr> {
public:
    using RoomSettingMap = std::unordered_map<RoomID, RoomSetiing>;
    using AccountSettingMap = std::unordered_map<AccountID, RobotSetting>;
    using RoomDataMap = std::unordered_map<RoomID, HallRoomData>;

    using RobotMap = std::unordered_map<UserID, RobotPtr>;
    using RoomCurUsersMap = std::unordered_map<RoomID, CurUserCount>;

public:
    // ��ʼ|����
    bool Init();
    void Term();

    // ��������������
    TTueRet SendHallRequest(TReqstId nReqId, uint32_t& nDataLen, void *pData, TReqstId &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho = true, uint32_t wait_ms = REQ_TIMEOUT_INTERVAL);

    // ���������ýӿ�
    bool GetRobotSetting(int account, RobotSetting& robot_setting_);

    int32_t ApplyRobotForRoom(int32_t nGameId, int32_t nRoomId, int32_t nMaxCount); // second level
    int32_t ApplyRobotForRoom(int32_t nGameId, int32_t nRoomId, TInt32Vec accounts); //should be first level

    // ���������ݹ���
    uint32_t	  GetAccountSettingSize() { return account_setting_map_.size(); }
    RobotPtr GetRobotByToken(const EConnType& type, const TokenID& id);


    // ����ʱ��
    void	OnServerMainTimer(time_t nCurrTime);

    // RoomData
    bool	GetRoomData(int32_t nRoomId, OUT ROOM& room);
    uint32_t GetRoomDataLastTime(int32_t nRoomId);
    void	SetRoomData(int32_t nRoomId, const LPROOM& room);

protected:
    // ��ʼ�������ļ�
    bool InitSetting();

    bool	InitNotifyThreads();

    bool	InitEnterGameThreads();

    bool	InitConnectHall(bool bReconn = false);

    bool	InitGameRoomDatas();


    // ֪ͨ��Ϣ�̷߳���
    void	ThreadRunHallNotify();
    void	ThreadRunRoomNotify();
    void	ThreadRunGameNotify();
    void	ThreadRunEnterGame();

    // ֪ͨ��Ϣ����ص�
    void OnHallNotify(TReqstId nReqId, void* pDataPtr, int32_t nSize);
    void OnRoomNotify(RobotPtr client, TReqstId nReqId, void* pDataPtr, int32_t nSize);
    void OnGameNotify(RobotPtr client, TReqstId nReqId, void* pDataPtr, int32_t nSize);

    void OnHallRoomUsersOK(TReqstId nReqId, void* pDataPtr);
    void OnRoomRobotEnter(RobotPtr client, int32_t nTableNo, int32_t nChairNo, std::string sEnterWay);

    void OnCliDisconnHall(TReqstId nReqId, void* pDataPtr, int32_t nSize);
    void OnCliDisconnRoom(RobotPtr client, TReqstId nReqId, void* pDataPtr, int32_t nSize);
    void OnCliDisconnGame(RobotPtr client, TReqstId nReqId, void* pDataPtr, int32_t nSize);

    // ��ʱ���ص�����
    bool	OnTimerReconnectHall(time_t nCurrTime);
    void    OnTimerSendHallPluse(time_t nCurrTime);
    void    OnTimerSendRoomPluse(time_t nCurrTime);
    void    OnTimerSendGamePluse(time_t nCurrTime);
    void    OnTimerCtrlRoomActiv(time_t nCurrTime);
    void    OnTimerUpdateDeposit(time_t nCurrTime);

    // ���縨������
    TTueRet	RobotLogonHall(const int32_t& account = 0);
    TTueRet SendGetRoomData(const int32_t nRoomId);
    TTueRet RobotGainDeposit(RobotPtr client);
    TTueRet RobotBackDeposit(RobotPtr client);

    //@zhuhangmin
    bool IsLogon(UserID userid);
    void SetLogon(UserID userid, bool status);
    RobotPtr GetRobotClient(UserID userid);
    void SetRobotClient(RobotPtr client);

    bool IsInRoom(UserID userid);
    void SetRoomID(UserID userid, RoomID roomid);

    int GetRoomCurrentRobotSize(RoomID roomid); //��ǰ��ĳ��������Ļ�������

protected:
    SINGLETION_CONSTRUCTOR(CRobotMgr);

    UThread			m_thrdHallNotify;
    UThread			m_thrdRoomNotify;
    UThread			m_thrdGameNotify;

    UThread			m_thrdEnterGames[DEF_ENTER_GAME_THREAD_NUM];


    //immutable������ ���ã��ȸ���,�������),��ʼ����,���ı�״̬,���ü���
    AccountSettingMap	account_setting_map_; // �������˺�����  robot.setting
    RoomSettingMap room_setting_map_; //�����˷������� robot.setting
    RoomDataMap	m_mapRoomData; // ������������ ������������� 

    //mutable������ �����
    std::mutex        robot_map_mutex_;
    RobotMap robot_map_; //��������robot��̬��Ϣ

    std::mutex hall_connection_mutex_;
    CDefSocketClientPtr hall_connection_{std::make_shared<CDefSocketClient>()};//��������
    RoomCurUsersMap room_cur_users_; //�������е�ǰ�����(���������˺�����)


};
#define TheRobotMgr CRobotMgr::Instance()