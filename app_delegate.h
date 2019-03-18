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
    // 启动时 为已在游戏中的机器人 立即建立大厅连接
    int ConnectHallForAllRobot();

    // 启动时 大厅登陆
    int LogonHall(const UserID& userid) const;

    // 启动时 大厅获取用户信息
    int GetUserGameInfo(const UserID& userid) const;

    // 启动时 为已在游戏中的机器人 立即建立游戏连接
    int ConnectGameForRobotInGame();

    // 过滤：获得未开桌+椅子等待 真实玩家集合
    int GetWaittingUser(UserMap& filter_user_map) const;

    // 0 随机选一个 1符合房银 2符合桌银 3没进入游戏 4无异常
    int GetRandomQualifiedRobotUserID(const RoomID& roomid, const TableNO& tableno, UserID& random_userid) const;

    // 获得此房间需要机器人匹配的用户信息
    int GetRoomNeedUserMap(UserMap& need_user_map);

    // 进入游戏
    int EnterGame(const UserID& userid, const RoomID& roomid, const TableNO& tableno);

    // 过滤: 桌银区间的机器人
    int FilterRobotNotInTableDepositRange(const RoomID& roomid, const TableNO& tableno, RobotUserIDMap& not_logon_game_temp) const;

    // 过滤: 符合房银区间的机器人
    int FilterRobotNotInRoomDepositRange(RobotUserIDMap& not_logon_game_temp) const;

public:
#ifdef _DEBUG
    //TEST ONLY
    void SendTestRobot(const UserID& userid, const RoomID& roomid, const TableNO& tableno);

    int TestLogonHall(const UserID& userid) const;
#endif

private:
    // 主流程 单线程 无业务锁
    // 允许脏读数据层data, 但不允许把脏读数据写回数据层data, 只读不写
    YQThread	timer_thread_;

    RobotUserIDMap robot_in_other_game_map_;
};
