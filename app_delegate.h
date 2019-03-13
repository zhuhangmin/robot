#pragma once
#include "robot_define.h"
#include "user_manager.h"

// 游戏业务
class AppDelegate {
public:
    int Init();

    int Term();

protected:
    int InitLanuch() const;

private:
    // 业务线程函数 
    int ThreadMainProc();

    // 业务主流程
    int MainProcess();

    // 机器人流程 登入大厅 进入游戏
    int RobotProcess(const UserID& userid, const RoomID& roomid, const TableNO& tableno);

private:
    // 辅助函数

    // 随机选一个没有进入游戏的userid
    int GetRandomUserIDNotInGame(UserID& random_userid) const;

    // 获得此房间需要机器人匹配的用户信息
    int GetRoomNeedUserMap(UserMap& need_user_map);

    // 启动时 集体补银
    int DepositGainAll();

    // 启动时 为已在游戏中的机器人 立即建立大厅连接
    int ConnectHallForAllRobot();

    // 启动时 为已在游戏中的机器人 立即建立游戏连接
    int ConnectGameForRobotInGame();

    // 登陆大厅
    int LogonHall(const UserID& userid);

    // 进入游戏
    int EnterGame(const UserID& userid, const RoomID& roomid, const TableNO& tableno);

    // 过滤：获得未开桌+椅子等待 真实玩家集合
    int GetWaittingUser(UserMap& filter_user_map);

public:
    //TEST ONLY
    void SendTestRobot(const UserID& userid, const RoomID& roomid, const TableNO& tableno);

    //TEST ONLY
    int TestLogonHall(const UserID& userid);

private:
    // 主流程 单线程 无业务锁
    // 允许脏读数据层data, 但不允许把脏读数据写回数据层data, 只读不写
    YQThread	main_timer_thread_;

    bool inited_{false};
};
