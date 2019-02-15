#pragma once
#include "RobotDef.h"
#include "RobotClient.h"

// �����˹�����
class CRobotMgr : public ISingletion<CRobotMgr> {
public:
    //@zhuhangmin TODO ������Բ��������ࣨ�ȸ��£��� ��̬����,  ֱ��Ӱ�����ķ�Χ
    typedef std::unordered_map<AccountID, stRobotUnit>			AccountSettingMap;

    typedef std::unordered_set<CRobotClient*>					RobotSet;
    typedef std::unordered_map<UserID, CRobotClient*>			LogonHallMap;
    typedef std::unordered_map<AccountID, CRobotClient*>		AccountRobotMap;

    typedef std::unordered_map<RoomID, stActiveCtrl>			RoomSettingMap;
    typedef std::unordered_map<RoomID, RobotSet>			    RoomRobotSetMap;
    typedef std::unordered_map<RoomID, stRoomData>				RoomDataMap;

    using RobotMap = std::unordered_map<UserID, CRobotClient*>;
    using RoomCurUsersMap = std::unordered_map<RoomID, int32_t>

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
    CRobotClient* GetRobotClient_UserId(const UserID& id);
    CRobotClient* GetRobotClient_Accout(const int32_t& account);


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

protected:
    SINGLETION_CONSTRUCTOR(CRobotMgr);

    CCritSec        m_csRobot;
    AccountSettingMap	account_setting_map_; // �˺���Ϣ  from robot.setting

    CCritSec        m_csRoomData;
    RoomDataMap	m_mapRoomData; // ������������

    LogonHallMap	logon_hall_map_; // ��½�����ɹ�����

    CCritSec        m_csTknRobot;
    AccountRobotMap   account_robot_map_; // ��½�����ɹ�����
    RoomRobotSetMap   room_robot_set_; //���뷿��ɹ�����
    RoomRobotSetMap   room_want_robot_map_;//�뷿��ʧ�ܵĻ�����,�ȴ��ؽ�

    CCritSec        m_csConnHall;
    CDefSocketClient* m_ConnHall{new CDefSocketClient()};

    UThread			m_thrdHallNotify;
    UThread			m_thrdRoomNotify;
    UThread			m_thrdGameNotify;

    CCritSec        m_csWaitEnters;
    UThread			m_thrdEnterGames[DEF_ENTER_GAME_THREAD_NUM];

    //@zhuhangmin
    RoomSettingMap room_setting_map_; //�����˷������� robot.setting
    RobotMap robot_map_; //@zhuhangmin 20190215 ��������robot��Ϣ
    RoomCurUsersMap room_cur_users_; //�����е�ǰ�����
};
#define TheRobotMgr CRobotMgr::Instance()