#pragma once
#include "RobotDef.h"
#include "RobotClient.h"

// 机器人管理器
class CRobotMgr : public ISingletion<CRobotMgr> {
public:
    using RoomSettingMap = std::unordered_map<RoomID, stActiveCtrl>;
    using AccountSettingMap = std::unordered_map<AccountID, stRobotUnit>;
    using RoomDataMap = std::unordered_map<RoomID, stRoomData>;

    using RobotMap = std::unordered_map<UserID, CRobotClient*>;//TODO 所有RobotClient 的类都是cache 需要合并重构
    using RoomCurUsersMap = std::unordered_map<RoomID, CurUserCount>;

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
    CRobotClient* GetRobotClient(UserID userid);
    void SetRobotClient(CRobotClient* client);

    bool IsInRoom(UserID userid);
    void SetRoomID(UserID userid, RoomID roomid);

    int GetRoomCurrentRobotSize(RoomID roomid); //当前在某个房间里的机器人数

protected:
    SINGLETION_CONSTRUCTOR(CRobotMgr);

    CCritSec        m_csConnHall;
    CDefSocketClient* m_ConnHall{new CDefSocketClient()};

    UThread			m_thrdHallNotify;
    UThread			m_thrdRoomNotify;
    UThread			m_thrdGameNotify;

    CCritSec        m_csWaitEnters;
    UThread			m_thrdEnterGames[DEF_ENTER_GAME_THREAD_NUM];

    //immutable数据类 配置（热更新,允许脏读),初始化后,不改变状态,不用加锁
    CCritSec        m_csRobot;
    AccountSettingMap	account_setting_map_; // 机器人账号配置  robot.setting
    RoomSettingMap room_setting_map_; //机器人房间配置 robot.setting
    RoomDataMap	m_mapRoomData; // 大厅房间配置 大厅获得 

    //mutable数据类 需加锁
    CCritSec        m_csRoomData;
    CCritSec        m_csTknRobot;
    RobotMap robot_map_; //管理所有robot信息
    RoomCurUsersMap room_cur_users_; //房间中当前玩家数(包括机器人和真人)


};
#define TheRobotMgr CRobotMgr::Instance()