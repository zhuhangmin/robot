#pragma once
#include "RobotDef.h"
#include "Robot.h"

// �����˹�����
class CRobotMgr : public ISingletion<CRobotMgr> {
public:
    //setting
    using RoomSettingMap = std::unordered_map<RoomID, RoomSetiing>;
    using AccountSettingMap = std::unordered_map<UserID, RobotSetting>;

    //game info
    using RoomMap = std::unordered_map<RoomID, game::base::Room>;
    using UserMap = std::unordered_map<UserID, game::base::User>;

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

    // ���������ݹ���
    uint32_t GetAccountSettingSize() { return account_setting_map_.size(); }
    RobotPtr GetRobotByToken(const EConnType& type, const TokenID& id);


    // ����ʱ��
    void	OnServerMainTimer(time_t nCurrTime);

protected:
    // ��ʼ�������ļ�
    bool InitSetting();

    bool	InitNotifyThreads();

    bool	InitEnterGameThreads();

    bool	InitConnectHall(bool bReconn = false);

    int InitConnectGame();

public:
    //@zhuhangmin new pb
    int SendValidateReq();
    int SendGetGameInfo(RoomID roomid = 0);
    //@zhuhangmin new pb

    // ֪ͨ��Ϣ�̷߳���
    void	ThreadRunHallNotify();
    void	ThreadRunGameNotify();
    void	ThreadRunGameInfoNotify();
    //TODO KEEPALIVE THREAD
public:
    void	ThreadRunEnterGame();

protected:
    // ֪ͨ��Ϣ����ص�
    void OnHallNotify(RequestID nReqId, void* pDataPtr, int32_t nSize);
    void OnGameNotify(RobotPtr client, RequestID nReqId, void* pDataPtr, int32_t nSize);
    void OnGameInfoNotify(RequestID nReqstID, const REQUEST &request);
    //TODO KEEPALIVE THREAD NOTIFY

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

    bool IsInRoom(UserID userid);
    void SetRoomID(UserID userid, RoomID roomid);

    int GetRoomCurrentRobotSize(RoomID roomid); //��ǰ��ĳ��������Ļ�������

public:
    int GetUserStatus(UserID userid, UserStatus& user_status);
    int FindTable(UserID userid, game::base::Table& table);
    int FindChair(UserID userid, game::base::ChairInfo& chair);

private:
    void OnDisconnHallWithLock(RequestID nReqId, void* pDataPtr, int32_t nSize);
    void OnDisconnGameWithLock(RobotPtr client, RequestID nReqId, void* pDataPtr, int32_t nSize);
    void OnDisconnGameInfo();

    void OnRecvGameInfo(const REQUEST &request);
protected:
    SINGLETION_CONSTRUCTOR(CRobotMgr);

    UThread			m_thrdHallNotify;
    UThread			m_thrdGameNotify;
    UThread			m_thrdGameInfoNotify;

    UThread			m_thrdEnterGames[DEF_ENTER_GAME_THREAD_NUM];


    // immutable������: ��ʼ���󲻸ı�״̬ ,���ü��� �������ȸ���,�������)
    // �������˺�����  robot.setting
    AccountSettingMap	account_setting_map_;

    // �����˷������� robot.setting
    RoomSettingMap room_setting_map_;


    // mutable������ �����
    std::mutex        robot_map_mutex_;
    RobotMap robot_map_; //��������robot��̬��Ϣ

    std::mutex hall_connection_mutex_;
    CDefSocketClientPtr hall_connection_{std::make_shared<CDefSocketClient>()};//��������

    std::mutex game_info_connection_mutex_; // ����ͬ����Ϸ������������Ϣ
    CDefSocketClientPtr game_info_connection_{std::make_shared<CDefSocketClient>()};//��Ϸ����
    RoomMap room_map_;
    UserMap user_map_;



};
#define TheRobotMgr CRobotMgr::Instance()