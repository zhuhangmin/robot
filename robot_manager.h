#pragma once
#include "robot_define.h"
#include "robot_game.h"

// 机器人管理器
class CRobotMgr : public ISingletion<CRobotMgr> {
public:
    using DepositMap = std::unordered_map<UserID, DepositType>;
    using RobotMap = std::unordered_map<UserID, RobotPtr>;
    using HallLogonMap = std::unordered_map<UserID, HallLogonStatusType>;
    using HallRoomDataMap = std::unordered_map<RoomID, HallRoomData>;

public:
    int Init();

    void Term();

protected:
    SINGLETION_CONSTRUCTOR(CRobotMgr);

private:
    // 大厅 建立连接
    int ConnectHall(bool bReconn = false);

    // 大厅 消息发送
    int SendHallRequestWithLock(RequestID requestid, int& data_size, void *req_data_ptr, RequestID &response_id, std::shared_ptr<void> &resp_data_ptr, bool need_echo = true);

    // 大厅 消息接收
    void ThreadHallNotify();

    // 大厅 消息处理
    void OnHallNotify(RequestID requestid, void* ntf_data_ptr, int data_size);

    // 大厅 断开链接
    void OnDisconnHall(RequestID requestid, void* data_ptr, int data_size);

    // 大厅 定时心跳
    void ThreadHallPluse();

    // 机器人 定时心跳
    void ThreadRobotPluse();

    // 机器人 消息接收
    void ThreadRobotNotify();

    // 机器人 消息处理
    void OnRobotNotify(RequestID requestid, void* ntf_data_ptr, int data_size, TokenID token_id);

    // 补银还银 定时触发
    void ThreadDeposit();

    // 业务主流程
    void ThreadMainProc();

private:
    //具体业务

    // 大厅登陆网络辅助请求
    int LogonHall(UserID userid);
    void SendHallPluse();
    int SendGetRoomData(RoomID roomid);

    // game
    void    SendGamePluse();

    //@zhuhangmin 20190218 仅补银线程可见
    int RobotGainDeposit(UserID userid, int amount);
    int RobotBackDeposit(UserID userid, int amount);

    // 机器人
    RobotPtr GetRobotWithLock(UserID userid);
    void SetRobotWithLock(RobotPtr robot);
    RobotPtr GetRobotByTokenWithLock(const TokenID& id);

private:
    //guard by lock
    int GetLogonStatusWithLock(const UserID& userid, HallLogonStatusType& status);
    void SetLogonStatusWithLock(const UserID userid, HallLogonStatusType status);

    int GetDepositTypeWithLock(const UserID& userid, DepositType& type);
    void SetDepositTypesWithLock(const UserID userid, DepositType type);

    int GetHallRoomData(const RoomID& roomid, HallRoomData& hall_room_data);
    int GetHallRoomDataWithLock(const RoomID& roomid, HallRoomData& hall_room_data);
    void SetHallRoomDataWithLock(const RoomID roomid, HallRoomData* hall_room_data);

private:
    // 随机选择没有登陆大厅的机器人
    int GetRandomNotLogonUserID(UserID& random_userid);

private:
    // 根据数据状态来源 划分 数据和锁范围 
    // 1 大厅, 2 游戏, 3 后台补银

    //大厅 数据锁
    std::mutex hall_connection_mutex_;
    //大厅 连接
    CDefSocketClientPtr hall_connection_{std::make_shared<CDefSocketClient>()};
    //大厅 登陆状态
    HallLogonMap hall_logon_status_map_;
    //大厅 房间配置
    HallRoomDataMap hall_room_data_map_;
    //大厅 接收消息线程
    YQThread	hall_notify_thread_;
    //大厅 心跳线程
    YQThread	hall_heart_timer_thread_;

    //机器人 数据锁
    std::mutex robot_map_mutex_;
    //机器人 游戏服务器连接集合
    RobotMap robot_map_;
    //机器人 接收消息线程
    YQThread	robot_notify_thread_;
    //机器人 心跳线程
    YQThread	robot_heart_timer_thread_;

    //后台补银 数据锁
    std::mutex deposit_map_mutex_;
    //后台补银 任务
    DepositMap deposit_map_;
    //后台补银 定时器线程
    YQThread	deposit_timer_thread_;

    //主流程 线程
    YQThread	main_timer_thread_;

};
