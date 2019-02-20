#pragma once
#include "RobotDef.h"
#include "Robot.h"

// 机器人管理器
class CRobotMgr : public ISingletion<CRobotMgr> {
public:


    //robot
    using RobotMap = std::unordered_map<UserID, RobotPtr>;
    using RoomCurUsersMap = std::unordered_map<RoomID, CurUserCount>;

public:
    // 开始|结束
    bool Init();
    void Term();

    // 大厅服务请求发送
    TTueRet SendHallRequest(RequestID nReqId, uint32_t& nDataLen, void *pData, RequestID &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho = true, uint32_t wait_ms = REQ_TIMEOUT_INTERVAL);




    RobotPtr GetRobotByToken(const EConnType& type, const TokenID& id);


    // 主定时器
    void	OnServerMainTimer(time_t nCurrTime);

protected:

    bool	InitNotifyThreads();

    bool	InitEnterGameThreads();

    bool	InitConnectHall(bool bReconn = false);



public:

    // 通知消息线程方法
    void	ThreadRunHallNotify();


public:
    void	ThreadRunEnterGame();

protected:
    // 通知消息处理回调
    void OnHallNotify(RequestID nReqId, void* pDataPtr, int32_t nSize);
    void OnGameNotify(RobotPtr client, RequestID nReqId, void* pDataPtr, int32_t nSize);

    //TODO KEEPALIVE THREAD NOTIFY

    // 定时器回调方法
    bool	OnTimerLogonHall(time_t nCurrTime);
    /*   void    OnTimerSendHallPluse(time_t nCurrTime);
       void    OnTimerSendGamePluse(time_t nCurrTime);*/

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

    void SetRoomID(UserID userid, RoomID roomid);

    int GetRoomCurrentRobotSize(RoomID roomid); //当前在某个房间里的机器人数

private:
    void OnDisconnHallWithLock(RequestID nReqId, void* pDataPtr, int32_t nSize);
    void OnDisconnGameWithLock(RobotPtr client, RequestID nReqId, void* pDataPtr, int32_t nSize);

protected:
    SINGLETION_CONSTRUCTOR(CRobotMgr);

    UThread			m_thrdHallNotify;
    UThread			m_thrdEnterGames[DEF_ENTER_GAME_THREAD_NUM];

    // mutable数据类 需加锁
    std::mutex        robot_map_mutex_;
    RobotMap robot_map_; //管理所有robot动态信息

    std::mutex hall_connection_mutex_;
    CDefSocketClientPtr hall_connection_{std::make_shared<CDefSocketClient>()};//大厅连接


};
#define TheRobotMgr CRobotMgr::Instance()