#pragma once
#include "RobotDef.h"
#include "RobotClient.h"

// 机器人管理器
class CRobotMgr : public ISingletion<CRobotMgr> {
public:
    typedef std::unordered_set<CRobotClient*>					TRobotCliSet;
    typedef std::unordered_map<int32_t, stActiveCtrl>			TRoomActivMap;
    typedef std::unordered_map<TUserId, CRobotClient*>			TUIdRobotMap;	// uid
    typedef std::unordered_map<TTokenId, CRobotClient*>			ToknRobotMap;	// token
    typedef std::unordered_map<int32_t, TRobotCliSet>			TRoomRobotMap;	// token
    typedef std::unordered_map<int32_t, CRobotClient*>			TAntRobotMap;	// account
    typedef std::unordered_map<int32_t, stRobotUnit>			TAcntSettMap;	// setting
    typedef std::unordered_map<int32_t, TRoomActivMap>			TGRoomSettMap;	// support
    typedef std::unordered_map<int32_t, stRoomData>				TRoomDataMap;	// roomdata
    typedef std::queue<CRobotClient*>							TRobotCliQue;

public:
    // 开始|结束
    bool Init();
    void Term();

    // 大厅服务请求发送
    TTueRet SendHallRequest(TReqstId nReqId, uint32_t& nDataLen, void *pData, TReqstId &nRespId, void* &pRetData, bool bNeedEcho = true, uint32_t wait_ms = REQ_TIMEOUT_INTERVAL);
    TTueRet SendHallRequestEx(TReqstId nReqId, TReqstId nSubReqId, uint32_t& nDataLen, void *pData, TReqstId &nRespId, void* &pRetData, bool bNeedEcho = true, uint32_t wait_ms = REQ_TIMEOUT_INTERVAL);

    // 机器人配置接口
    bool	GetRobotSetting(int account, OUT stRobotUnit& unit);
    bool	UpdateRobotSetting(const stRobotUnit& unit);
    bool    UpdateGRoomSetting(const int32_t nGameId, const stActiveCtrl & active);

    int32_t ApplyRobotForRoom(int32_t nGameId, int32_t nRoomId, int32_t nMaxCount);
    int32_t ApplyRobotForRoom(int32_t nGameId, int32_t nRoomId, TInt32Vec accounts);

    // 机器人数据管理
    uint32_t	  GetAcntSettingSize() { return m_mapAcntSett.size(); }
    CRobotClient* GetRobotClient_ToknId(const EConnType& type, const TTokenId& id);
    CRobotClient* GetRobotClient_UserId(const TUserId& id);
    CRobotClient* GetRobotClient_Accout(const int32_t& account);


    // 主定时器
    void	OnServerMainTimer(time_t nCurrTime);

    // RoomData
    bool	GetRoomData(int32_t nRoomId, OUT ROOM& room);
    uint32_t GetRoomDataLastTime(int32_t nRoomId);
    void	SetRoomData(int32_t nRoomId, const LPROOM& room);

    // WaitEnter
    void			AddWaitEnter(CRobotClient* pRoboter);
    CRobotClient*	GetWaitEnter();

    void    UpdRobotClientToken(const EConnType& type, CRobotClient* client, bool add);
protected:
    // 初始化配置文件
    bool	InitSetting(std::string filename);

    bool	InitNotifyThreads();

    bool	InitEnterGameThreads();

    bool	InitConnectHall(bool bReconn = false);

    bool	InitGameRoomDatas();



    void	SaveSettingFile();

    // 通知消息线程方法
    void	ThreadRunHallNotify();
    void	ThreadRunRoomNotify();
    void	ThreadRunGameNotify();
    void	ThreadRunEnterGame();

    // 通知消息处理回调
    void	OnHallNotify(CRobotClient* client, TReqstId nReqId, void* pDataPtr, int32_t nSize);
    void	OnRoomNotify(CRobotClient* client, TReqstId nReqId, void* pDataPtr, int32_t nSize);
    void	OnGameNotify(CRobotClient* client, TReqstId nReqId, void* pDataPtr, int32_t nSize);

    void	OnHallRoomUsersOK(TReqstId nReqId, void* pDataPtr);
    void	OnRoomRobotEnter(CRobotClient* client, int32_t nTableNo, int32_t nChairNo, std::string sEnterWay);

    void	OnCliDisconnHall(CRobotClient* client, TReqstId nReqId, void* pDataPtr, int32_t nSize);
    void    OnCliDisconnRoom(CRobotClient* client, TReqstId nReqId, void* pDataPtr, int32_t nSize);
    void    OnCliDisconnGame(CRobotClient* client, TReqstId nReqId, void* pDataPtr, int32_t nSize);

    // 定时器回调方法
    bool	OnTimerReconnectHall(time_t nCurrTime);
    void    OnTimerSendHallPluse(time_t nCurrTime);
    void    OnTimerSendRoomPluse(time_t nCurrTime);
    void    OnTimerSendGamePluse(time_t nCurrTime);
    void    OnTimerCtrlRoomActiv(time_t nCurrTime);
    void    OnTimerUpdateDeposit(time_t nCurrTime);
    void    OnThrndDelyEnterGame(time_t nCurrTime);

    // 网络辅助请求
    TTueRet	RobotLogonHall(const int32_t& account = 0);
    TTueRet	SendGetRoomData(const int32_t nGameId, const int32_t nRoomId);
    TTueRet	RobotGainDeposit(CRobotClient* client);
    TTueRet	RobotBackDeposit(CRobotClient* client);

    //zhuhangmin
    int	GetIntervalTime(int curRoomUsers);
protected:
    SINGLETION_CONSTRUCTOR(CRobotMgr);

    CCritSec        m_csRobot;
    TAcntSettMap	m_mapAcntSett; // robot setting
    TGRoomSettMap   m_mapGRoomSett;

    CCritSec        m_csRoomData;
    TRoomDataMap	m_mapRoomData; // room data

    TUIdRobotMap	m_mapUIdRobot; // key : userid, value: robots already in hall

    CCritSec        m_csTknRobot;
    ToknRobotMap	m_mapTknRobot_Hall;
    ToknRobotMap	m_mapTknRobot_Room;
    ToknRobotMap	m_mapTknRobot_Game;

    TAntRobotMap	m_mapAntRobot; // key:account,  value: robots already in hall
    TRoomRobotMap   m_mapRoomRobot; //key:roomid ， value：robots already in roomid
    TRoomRobotMap   m_mapWantRoomRobot;//key:roomid ， value：robots enter fail and wait to retry

    CCritSec        m_csConnHall;
    CDefSocketClient* m_ConnHall{new CDefSocketClient()};

    UThread			m_thrdHallNotify;
    UThread			m_thrdRoomNotify;
    UThread			m_thrdGameNotify;

    CCritSec        m_csWaitEnters;
    TRobotCliQue	m_queWaitEnters; //已进入房间，等待进入游戏的机器人集合
    UThread			m_thrdEnterGames[DEF_ENTER_GAME_THREAD_NUM];
};
#define TheRobotMgr CRobotMgr::Instance()