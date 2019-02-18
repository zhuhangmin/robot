#pragma once
#include "RobotDef.h"
#include "RobotClient.h"

// �����˹�����
class CRobotMgr : public ISingletion<CRobotMgr> {
public:
    using RoomSettingMap = std::unordered_map<RoomID, stActiveCtrl>;
    using AccountSettingMap = std::unordered_map<AccountID, stRobotUnit>;
    using RoomDataMap = std::unordered_map<RoomID, stRoomData>;

    using RobotMap = std::unordered_map<UserID, CRobotClient*>;//TODO ����RobotClient ���඼��cache ��Ҫ�ϲ��ع�
    using RoomCurUsersMap = std::unordered_map<RoomID, CurUserCount>;

public:
    // ��ʼ|����
    bool Init();
    void Term();

    // ��������������
    TTueRet SendHallRequest(TReqstId nReqId, uint32_t& nDataLen, void *pData, TReqstId &nRespId, void* &pRetData, bool bNeedEcho = true, uint32_t wait_ms = REQ_TIMEOUT_INTERVAL);

    // ���������ýӿ�
    bool	GetRobotSetting(int account, OUT stRobotUnit& unit);

    int32_t ApplyRobotForRoom(int32_t nGameId, int32_t nRoomId, int32_t nMaxCount); // second level
    int32_t ApplyRobotForRoom(int32_t nGameId, int32_t nRoomId, TInt32Vec accounts); //should be first level

    // ���������ݹ���
    uint32_t	  GetAcntSettingSize() { return account_setting_map_.size(); }
    CRobotClient* GetRobotClient_ToknId(const EConnType& type, const TokenID& id);


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
    void	OnRoomNotify(CRobotClient* client, TReqstId nReqId, void* pDataPtr, int32_t nSize);
    void	OnGameNotify(CRobotClient* client, TReqstId nReqId, void* pDataPtr, int32_t nSize);

    void	OnHallRoomUsersOK(TReqstId nReqId, void* pDataPtr);
    void	OnRoomRobotEnter(CRobotClient* client, int32_t nTableNo, int32_t nChairNo, std::string sEnterWay);

    void OnCliDisconnHall(TReqstId nReqId, void* pDataPtr, int32_t nSize);
    void    OnCliDisconnRoom(CRobotClient* client, TReqstId nReqId, void* pDataPtr, int32_t nSize);
    void    OnCliDisconnGame(CRobotClient* client, TReqstId nReqId, void* pDataPtr, int32_t nSize);

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
    TTueRet	RobotGainDeposit(CRobotClient* client);
    TTueRet	RobotBackDeposit(CRobotClient* client);

    //@zhuhangmin
    bool IsLogon(UserID userid);
    void SetLogon(UserID userid, bool status);
    CRobotClient* GetRobotClient(UserID userid);
    void SetRobotClient(CRobotClient* client);

    bool IsInRoom(UserID userid);
    void SetRoomID(UserID userid, RoomID roomid);

    int GetRoomCurrentRobotSize(RoomID roomid); //��ǰ��ĳ��������Ļ�������

protected:
    SINGLETION_CONSTRUCTOR(CRobotMgr);

    CCritSec        m_csConnHall;
    CDefSocketClient* m_ConnHall{new CDefSocketClient()};

    UThread			m_thrdHallNotify;
    UThread			m_thrdRoomNotify;
    UThread			m_thrdGameNotify;

    CCritSec        m_csWaitEnters;
    UThread			m_thrdEnterGames[DEF_ENTER_GAME_THREAD_NUM];

    //immutable������ ���ã��ȸ���,�������),��ʼ����,���ı�״̬,���ü���
    CCritSec        m_csRobot;
    AccountSettingMap	account_setting_map_; // �������˺�����  robot.setting
    RoomSettingMap room_setting_map_; //�����˷������� robot.setting
    RoomDataMap	m_mapRoomData; // ������������ ������� 

    //mutable������ �����
    CCritSec        m_csRoomData;
    CCritSec        m_csTknRobot;
    RobotMap robot_map_; //��������robot��Ϣ
    RoomCurUsersMap room_cur_users_; //�����е�ǰ�����(���������˺�����)


};
#define TheRobotMgr CRobotMgr::Instance()