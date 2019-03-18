#pragma once
#include "robot_define.h"

class RobotHallManager : public ISingletion<RobotHallManager> {
public:
    // 无WithLock后缀函数：如果方法有多线程可见数据 一般都需要加锁，除非业务层次允许脏读
    // 组合 lock + WithLock 函数组合而成, 保证不会获得多次mutex，std::mutex 为非递归锁
    // 只对外部线程可见
    int Init();

    int Term();

    // 大厅 登陆
    int LogonHall(const UserID& userid);

    // 大厅 获得用户信息
    int GetUserGameInfo(const UserID& userid);

    // 大厅 设置登陆状态
    int SetLogonStatus(const UserID& userid, const HallLogonStatusType& status);

    // 大厅 补银还银状态
    int SetDepositUpdate(const UserID& userid);

    // 获得大厅房间数据 返回 hall_room_data COPY
    int GetHallRoomData(const RoomID& roomid, HallRoomData& hall_room_data);

    // 随机选择没有登陆大厅的机器人
    int GetRandomNotLogonUserID(UserID& random_userid);

private:
    // 大厅 定时消息
    int ThreadTimer();

    // 大厅 更新机器人银子信息
    int SendUpdateUserDeposit();

    // 大厅 消息接收
    int ThreadNotify();

    // 大厅 消息处理
    int OnHallNotify(const RequestID& requestid, void* ntf_data_ptr, const int& data_size);

    // 大厅 断开链接
    int OnDisconnect();

    // 大厅 心跳
    int SendPulse();

    // 大厅 所有房间数据
    int SendGetAllRoomData();

private:
    // 辅助函数 WithLock 标识调用前需要获得此对象的数据锁mutex

    // 大厅 建立连接
    int ConnectWithLock() const;

    // 大厅 消息发送
    int SendRequestWithLock(const RequestID& requestid, int& data_size, void *req_data_ptr, RequestID &response_id, std::shared_ptr<void> &resp_data_ptr, const bool& need_echo = true) const;

    // 大厅 获得登陆状态
    int GetLogonStatusWithLock(const UserID& userid, HallLogonStatusType& status) const;

    int SetLogonStatusWithLock(const UserID& userid, const HallLogonStatusType& status);

    // 大厅 获得房间数据
    int GetHallRoomDataWithLock(const RoomID& roomid, HallRoomData& hall_room_data) const;

    // 大厅 设置房间数据
    int SetHallRoomDataWithLock(const RoomID& roomid, HallRoomData* hall_room_data);

    // 大厅 初始化连接和数据
    int InitDataWithLock();

    // 大厅 重置连接和数据
    int ResetDataWithLock();

    // 大厅 所有房间数据
    int SendGetAllRoomDataWithLock();

    // 大厅 房间数据
    int SendGetRoomDataWithLock(const RoomID& roomid);

    // 大厅 获得用户信息
    int GetUserGameInfoWithLock(const UserID& userid);

    // 连接保活
    int KeepConnection();

public:
    // 非业务 调试函数
    // 对象状态快照
    int SnapShotObjectStatus();

    int BriefInfo() const;

private:
    // 方法的线程可见性检查
    int CheckNotInnerThread();

protected:
    SINGLETION_CONSTRUCTOR(RobotHallManager);
private:
    // 保持一个对象一把锁对应关系,防止多把锁嵌套造成死锁
    // 发现必须有多把锁时说明对象数据聚合太多，需要分成多个对象

    //大厅 数据锁
    mutable std::mutex mutex_;

    //大厅 连接
    CDefSocketClientPtr connection_{std::make_shared<CDefSocketClient>()};

    //大厅 登陆状态
    HallLogonMap logon_status_map_;

    //大厅 房间配置
    HallRoomDataMap room_data_map_;

    //大厅 接收消息线程
    YQThread	notify_thread_;

    //大厅 心跳线程
    YQThread	timer_thread_;

    //大厅 超时计数
    int timeout_count_{0};

    //大厅 是否需要重连标签
    bool need_reconnect_{false};

    //大厅 更新机器人银子 任务
    RobotUserIDMap update_depost_map_;

    //大厅 心跳时间戳
    TimeStamp timestamp_{std::time(nullptr)};

    //大厅 房间信息同步时间戳
    TimeStamp room_data_timestamp_{std::time(nullptr)};

};

#define HallMgr  RobotHallManager::Instance()