#pragma once
#include "RobotDef.h"
#include "Robot.h"

// 机器人管理器
class CRobotMgr : public ISingletion<CRobotMgr> {
public:
    using DepositMap = std::unordered_map<UserID, DepositType>;
    using RobotMap = std::unordered_map<UserID, RobotPtr>;
    using HallLogonStatusMap = std::unordered_map<UserID, HallLogonStatusType>;


public:
    int Init();

    void Term();

protected:
    SINGLETION_CONSTRUCTOR(CRobotMgr);

private:
    // 大厅 建立连接
    int ConnectHall(bool bReconn = false);

    // 大厅 消息发送
    int SendHallRequestWithLock(RequestID nReqId, uint32_t& nDataLen, void *pData, RequestID &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho = true, uint32_t wait_ms = REQ_TIMEOUT_INTERVAL);

    // 大厅 消息接收
    void ThreadHallNotify();

    // 大厅 消息处理
    void OnHallNotify(RequestID nReqId, void* pDataPtr, int32_t nSize);

    // 大厅 断开链接
    void OnDisconnHall(RequestID nReqId, void* pDataPtr, int32_t nSize);

    // 大厅 定时心跳
    void ThreadHallPluse();

    // 机器人 定时心跳
    void ThreadRobotPluse();

    // 定时 业务主流程
    void ThreadMainProc();

    // 定时 补银还银
    void ThreadDeposit();

private:
    //具体业务

    // 大厅登陆网络辅助请求
    int LogonHall(UserID userid);

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
    //guard by lock
    int GetLogonStatusWithLock(const UserID& userid, HallLogonStatusType& status);
    void SetLogonStatusWithLock(const UserID userid, HallLogonStatusType status);

    int GetDepositTypeWithLock(const UserID& userid, DepositType& type);
    void SetDepositTypesWithLock(const UserID userid, DepositType type);

private:
    int GetRandomNotLogonUserID(UserID& random_userid);

private:
    // 根据业务划分数据和锁 

    //大厅 数据锁
    std::mutex hall_connection_mutex_;
    //大厅 连接
    CDefSocketClientPtr hall_connection_{std::make_shared<CDefSocketClient>()};
    //大厅 登陆状态
    HallLogonStatusMap hall_logon_status_map_;
    //大厅 接收消息线程
    UThread	hall_notify_thread_;
    //大厅 心跳线程
    UThread	hall_heart_timer_thread_;

    //机器人 数据锁
    std::mutex robot_map_mutex_;
    //机器人 游戏服务器连接集合
    RobotMap robot_map_;
    //机器人 心跳线程
    UThread	robot_heart_timer_thread_;

    //补银 数据锁
    std::mutex deposit_map_mutex_;
    //补银 任务
    DepositMap deposit_map_;
    //补银 定时器线程
    UThread	deposit_timer_thread_;


    //主流程 定时器线程
    UThread	main_timer_thread_;

};
#define TheRobotMgr CRobotMgr::Instance()