#pragma once
#include "robot_define.h"

// 游戏业务
class AppDelegate {
public:
    AppDelegate();
    ~AppDelegate();

public:
    int Init();

    int Term();

protected:
    int InitLanuch();

private:
    // 业务线程函数 
    int ThreadMainProc();

    // 业务主流程
    int MainProcess();

    // 机器人流程 登入大厅 进入游戏
    int RobotProcess(UserID userid, RoomID roomid);

private:
    // 辅助函数

    // 随机选一个没有进入游戏的userid
    int GetRandomUserIDNotInGame(UserID& random_userid);

    // 获得房间此时需要的机器人数量
    int GetRoomNeedCountMap(RoomNeedCountMap& room_count_map);

    // 启动时集体补银
    int DepositGainAll();

private:
    // @zhuhangmin 20190228
    // 注意： 主流程 单线程
    // 无业务锁, 若有高并发需求，可以让机器人预先进游戏等待
    // 允许脏读数据层data, 但不允许把脏读数据写回数据层data, 只读不写
    YQThread	main_timer_thread_;
};
