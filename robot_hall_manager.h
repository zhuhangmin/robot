#pragma once
#include "robot_define.h"
class RobotHallManager : public ISingletion<RobotHallManager> {
public:
    using HallLogonMap = std::unordered_map<UserID, HallLogonStatusType>;
    using HallRoomDataMap = std::unordered_map<RoomID, HallRoomData>;
public:
    int Init();

    void Term();

public:
    // 大厅 业务

    // 大厅 登陆
    int LogonHall(UserID userid);

    // 大厅 心跳
    void SendHallPluse();

    // 大厅 房间数据
    int SendGetRoomData(RoomID roomid);

    // 大厅 所有房间数据
    int SendGetAllRoomData();

public:
    // 获得大厅房间数据
    int GetHallRoomData(const RoomID& roomid, HallRoomData& hall_room_data);

    // 随机选择没有登陆大厅的机器人
    int GetRandomNotLogonUserID(UserID& random_userid);

protected:
    SINGLETION_CONSTRUCTOR(RobotHallManager);

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

private:
    //guard by lock

    int GetLogonStatusWithLock(const UserID& userid, HallLogonStatusType& status);

    int SetLogonStatusWithLock(const UserID userid, HallLogonStatusType status);

    int GetHallRoomDataWithLock(const RoomID& roomid, HallRoomData& hall_room_data);

    int SetHallRoomDataWithLock(const RoomID roomid, HallRoomData* hall_room_data);

private:
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
};

#define HallMgr  RobotHallManager::Instance()