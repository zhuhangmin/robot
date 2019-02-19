#pragma once
#include "RobotDef.h"
#include "Robot.h"

// 机器人管理器
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
    // 开始|结束
    bool Init();
    void Term();

    // 大厅服务请求发送
    TTueRet SendHallRequest(RequestID nReqId, uint32_t& nDataLen, void *pData, RequestID &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho = true, uint32_t wait_ms = REQ_TIMEOUT_INTERVAL);

    // 机器人配置接口
    bool GetRobotSetting(int account, RobotSetting& robot_setting_);

    //int32_t ApplyRobotForRoom(int32_t nGameId, int32_t nRoomId, int32_t nMaxCount); // second level
    //int32_t ApplyRobotForRoom(int32_t nGameId, int32_t nRoomId, TInt32Vec accounts); //should be first level

    // 机器人数据管理
    uint32_t	  GetAccountSettingSize() { return account_setting_map_.size(); }
    RobotPtr GetRobotByToken(const EConnType& type, const TokenID& id);


    // 主定时器
    void	OnServerMainTimer(time_t nCurrTime);

    //// RoomData
    //bool	GetRoomData(int32_t nRoomId, OUT ROOM& room);
    //uint32_t GetRoomDataLastTime(int32_t nRoomId);
    //void	SetRoomData(int32_t nRoomId, const LPROOM& room);

protected:
    // 初始化配置文件
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

    // 通知消息线程方法
    void	ThreadRunHallNotify();
    void	ThreadRunRoomNotify();
    void	ThreadRunGameNotify();
    void	ThreadRunGameInfoNotify();
    void	ThreadRunEnterGame();

    // 通知消息处理回调
    void OnHallNotify(RequestID nReqId, void* pDataPtr, int32_t nSize);
    void OnRoomNotify(RobotPtr client, RequestID nReqId, void* pDataPtr, int32_t nSize);
    void OnGameNotify(RobotPtr client, RequestID nReqId, void* pDataPtr, int32_t nSize);

    void OnRoomRobotEnter(RobotPtr client, int32_t nTableNo, int32_t nChairNo, std::string sEnterWay);


    // 定时器回调方法
    bool	OnTimerLogonHall(time_t nCurrTime);
    void    OnTimerSendHallPluse(time_t nCurrTime);
    void    OnTimerSendRoomPluse(time_t nCurrTime);
    void    OnTimerSendGamePluse(time_t nCurrTime);
    void    OnTimerCtrlRoomActiv(time_t nCurrTime);

    //@zhuhangmin 20190218 仅补银线程可见 beg
    void    OnTimerUpdateDeposit(time_t nCurrTime);
    TTueRet RobotGainDeposit(RobotPtr client);
    TTueRet RobotBackDeposit(RobotPtr client);
    //@zhuhangmin 20190218 仅补银线程可见 end

    // 网络辅助请求
    TTueRet	RobotLogonHall(const int32_t& account = 0);


    //@zhuhangmin
    bool IsLogon(UserID userid);
    void SetLogon(UserID userid, bool status);
    RobotPtr GetRobotClient(UserID userid);
    void SetRobotClient(RobotPtr client);

    bool IsInRoom(UserID userid);
    void SetRoomID(UserID userid, RoomID roomid);

    int GetRoomCurrentRobotSize(RoomID roomid); //当前在某个房间里的机器人数

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


    //immutable数据类 配置（热更新,允许脏读),初始化后,不改变状态,不用加锁
    AccountSettingMap	account_setting_map_; // 机器人账号配置  robot.setting
    RoomSettingMap room_setting_map_; //机器人房间配置 robot.setting
    //RoomDataMap	m_mapRoomData; // 大厅房间配置 大厅服务器获得 

    //mutable数据类 需加锁
    std::mutex        robot_map_mutex_;
    RobotMap robot_map_; //管理所有robot动态信息

    std::mutex hall_connection_mutex_;
    CDefSocketClientPtr hall_connection_{std::make_shared<CDefSocketClient>()};//大厅连接

    std::mutex game_info_connection_mutex_; // 用于同步游戏服务器集合信息
    CDefSocketClientPtr game_info_connection_{std::make_shared<CDefSocketClient>()};//游戏连接
    RoomMap room_map_;



};
#define TheRobotMgr CRobotMgr::Instance()