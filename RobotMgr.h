#pragma once
#include "RobotDef.h"
#include "Robot.h"

// 机器人管理器
class CRobotMgr : public ISingletion<CRobotMgr> {
public:
    using RobotMap = std::unordered_map<UserID, RobotPtr>;
    using DepositMap = std::unordered_map<UserID, DepositType>;

public:
    int Init();

    void Term();

protected:
    SINGLETION_CONSTRUCTOR(CRobotMgr);

private:
    // 大厅 建立连接
    int ConnectHall(bool bReconn = false);

    // 大厅 消息发送
    int SendHallRequest(RequestID nReqId, uint32_t& nDataLen, void *pData, RequestID &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho = true, uint32_t wait_ms = REQ_TIMEOUT_INTERVAL);

    // 大厅 消息接收
    void ThreadHallNotify();

    // 大厅 消息处理
    void OnHallNotify(RequestID nReqId, void* pDataPtr, int32_t nSize);

    // 大厅 断开链接
    void OnDisconnHall(RequestID nReqId, void* pDataPtr, int32_t nSize);

    // 定时心跳
    void ThreadSendPluse();

    // 定时 业务流程
    void ThreadMainProc();

    // 定时 补银还银
    void ThreadDeposit();

private:
    //具体业务

    // 大厅登陆网络辅助请求
    int LogonHall();

    //@zhuhangmin 20190218 仅心跳线程可见
    void    OnTimerSendPluse(time_t nCurrTime);
    void    SendHallPluse();
    void    SendGamePluse();

    //@zhuhangmin 20190218 仅补银线程可见
    int RobotGainDeposit(UserID userid, int32_t amount);
    int RobotBackDeposit(UserID userid, int32_t amount);

    // 机器人
    RobotPtr GetRobotWithLock(UserID userid);

    void SetRobotWithLock(RobotPtr client);

    RobotPtr GetRobotByToken(const EConnType& type, const TokenID& id);

private:
    //主流程 定时器线程
    UThread	main_timer_thread_;

    //心跳 定时器线程
    UThread	heart_timer_thread_;

    //补银 定时器线程
    UThread	deposit_timer_thread_;

    //大厅 接收消息线程
    UThread	hall_notify_thread_;

    //所有机器人动态信息
    std::mutex robot_map_mutex_;
    RobotMap robot_map_;

    //大厅连接
    std::mutex hall_connection_mutex_;
    CDefSocketClientPtr hall_connection_{std::make_shared<CDefSocketClient>()};

    //补银队列
    std::mutex deposit_map_mutex_;
    DepositMap deposit_map_;


};
#define TheRobotMgr CRobotMgr::Instance()