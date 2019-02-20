#pragma once
#include "RobotDef.h"
#include "Robot.h"

// 机器人管理器
class CRobotMgr : public ISingletion<CRobotMgr> {
public:
    using RobotMap = std::unordered_map<UserID, RobotPtr>;

public:
    bool Init();

    void Term();

protected:
    SINGLETION_CONSTRUCTOR(CRobotMgr);

private:
    // 大厅建立连接
    bool ConnectHall(bool bReconn = false);

    // 大厅接收消息
    void ThreadHallNotify();

    // 大厅服务请求发送
    int SendHallRequest(RequestID nReqId, uint32_t& nDataLen, void *pData, RequestID &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho = true, uint32_t wait_ms = REQ_TIMEOUT_INTERVAL);

    // 大厅消息处理
    void OnHallNotify(RequestID nReqId, void* pDataPtr, int32_t nSize);

    // 大厅断开链接
    void OnDisconnHallWithLock(RequestID nReqId, void* pDataPtr, int32_t nSize);

    // 定时器 大厅心跳
    void ThreadSendPluse();

    // 定时器 主流程业务
    void ThreadMainProc();

    // 定时器 补银
    void ThreadDeposit();

private:
    //具体业务

    // 定时器 登陆大厅
    bool OnTimerLogonHall(time_t nCurrTime);

    // 大厅登陆网络辅助请求
    int RobotLogonHall(const int32_t& account = 0);

    //@zhuhangmin 20190218 仅心跳线程可见
    void    OnTimerSendPluse(time_t nCurrTime);
    void    SendHallPluse(time_t nCurrTime);
    void    SendGamePluse(time_t nCurrTime);

    //@zhuhangmin 20190218 仅补银线程可见
    void OnTimerUpdateDeposit(time_t nCurrTime);
    int RobotGainDeposit(RobotPtr client);
    int RobotBackDeposit(RobotPtr client);

    // 机器人
    RobotPtr GetRobotByToken(const EConnType& type, const TokenID& id);
    bool IsLogon(UserID userid);
    void SetLogon(UserID userid, bool status);
    RobotPtr GetRobotClient(UserID userid);
    void SetRobotClient(RobotPtr client);

    void SetRoomID(UserID userid, RoomID roomid);

    int GetRoomCurrentRobotSize(RoomID roomid); //房间里的机器人数


private:
    UThread	main_timer_thread_;

    UThread	heart_timer_thread_;

    UThread	deposit_timer_thread_;

    UThread	hall_notify_thread_;

    std::mutex robot_map_mutex_;
    RobotMap robot_map_; //管理所有robot动态信息

    std::mutex hall_connection_mutex_;
    CDefSocketClientPtr hall_connection_{std::make_shared<CDefSocketClient>()};//大厅连接




};
#define TheRobotMgr CRobotMgr::Instance()