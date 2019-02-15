#pragma once
#include "RobotDef.h"
#include "RobotClient.h"

// 机器人管理器
class CRobotMgr : public ISingletion<CRobotMgr> {
public:
    //@zhuhangmin TODO 区分相对不变配置类（热更新）和 动态数据,  直接影响锁的范围
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
    // 开始|结束
    bool Init();
    void Term();

    // 大厅服务请求发送
    TTueRet SendHallRequest(TReqstId nReqId, uint32_t& nDataLen, void *pData, TReqstId &nRespId, void* &pRetData, bool bNeedEcho = true, uint32_t wait_ms = REQ_TIMEOUT_INTERVAL);

    // 机器人配置接口
    bool	GetRobotSetting(int account, OUT stRobotUnit& unit);

    int32_t ApplyRobotForRoom(int32_t nGameId, int32_t nRoomId, int32_t nMaxCount); // second level
    int32_t ApplyRobotForRoom(int32_t nGameId, int32_t nRoomId, TInt32Vec accounts); //should be first level

    // 机器人数据管理
    uint32_t	  GetAcntSettingSize() { return account_setting_map_.size(); }
    CRobotClient* GetRobotClient_ToknId(const EConnType& type, const TokenID& id);
    CRobotClient* GetRobotClient_UserId(const UserID& id);
    CRobotClient* GetRobotClient_Accout(const int32_t& account);


    // 主定时器
    void	OnServerMainTimer(time_t nCurrTime);

    // RoomData
    bool	GetRoomData(int32_t nRoomId, OUT ROOM& room);
    uint32_t GetRoomDataLastTime(int32_t nRoomId);
    void	SetRoomData(int32_t nRoomId, const LPROOM& room);

protected:
    // 初始化配置文件
    bool InitSetting();

    bool	InitNotifyThreads();

    bool	InitEnterGameThreads();

    bool	InitConnectHall(bool bReconn = false);

    bool	InitGameRoomDatas();


    // 通知消息线程方法
    void	ThreadRunHallNotify();
    void	ThreadRunRoomNotify();
    void	ThreadRunGameNotify();
    void	ThreadRunEnterGame();

    // 通知消息处理回调
    void OnHallNotify(TReqstId nReqId, void* pDataPtr, int32_t nSize);
    void	OnRoomNotify(CRobotClient* client, TReqstId nReqId, void* pDataPtr, int32_t nSize);
    void	OnGameNotify(CRobotClient* client, TReqstId nReqId, void* pDataPtr, int32_t nSize);

    void	OnHallRoomUsersOK(TReqstId nReqId, void* pDataPtr);
    void	OnRoomRobotEnter(CRobotClient* client, int32_t nTableNo, int32_t nChairNo, std::string sEnterWay);

    void OnCliDisconnHall(TReqstId nReqId, void* pDataPtr, int32_t nSize);
    void    OnCliDisconnRoom(CRobotClient* client, TReqstId nReqId, void* pDataPtr, int32_t nSize);
    void    OnCliDisconnGame(CRobotClient* client, TReqstId nReqId, void* pDataPtr, int32_t nSize);

    // 定时器回调方法
    bool	OnTimerReconnectHall(time_t nCurrTime);
    void    OnTimerSendHallPluse(time_t nCurrTime);
    void    OnTimerSendRoomPluse(time_t nCurrTime);
    void    OnTimerSendGamePluse(time_t nCurrTime);
    void    OnTimerCtrlRoomActiv(time_t nCurrTime);
    void    OnTimerUpdateDeposit(time_t nCurrTime);

    // 网络辅助请求
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
    AccountSettingMap	account_setting_map_; // 账号信息  from robot.setting

    CCritSec        m_csRoomData;
    RoomDataMap	m_mapRoomData; // 大厅房间配置

    LogonHallMap	logon_hall_map_; // 登陆大厅成功集合

    CCritSec        m_csTknRobot;
    AccountRobotMap   account_robot_map_; // 登陆大厅成功集合
    RoomRobotSetMap   room_robot_set_; //进入房间成功集合
    RoomRobotSetMap   room_want_robot_map_;//入房间失败的机器人,等待重进

    CCritSec        m_csConnHall;
    CDefSocketClient* m_ConnHall{new CDefSocketClient()};

    UThread			m_thrdHallNotify;
    UThread			m_thrdRoomNotify;
    UThread			m_thrdGameNotify;

    CCritSec        m_csWaitEnters;
    UThread			m_thrdEnterGames[DEF_ENTER_GAME_THREAD_NUM];

    //@zhuhangmin
    RoomSettingMap room_setting_map_; //机器人房间配置 robot.setting
    RobotMap robot_map_; //@zhuhangmin 20190215 管理所有robot信息
    RoomCurUsersMap room_cur_users_; //房间中当前玩家数
};
#define TheRobotMgr CRobotMgr::Instance()