#pragma once
#include "robot_define.h"

class MainServer {
public:
    MainServer();
    ~MainServer();

public:
    int Init();

    int Term();

protected:
    int InitLanuch();

private:
    // 业务主流程
    int ThreadMainProc();

private:
    // 随机选一个没有进入游戏的userid
    int FindRandomUserIDNotInGame(UserID& random_userid);

private:
    //主流程 线程 （单线程 无业务锁需求, 若有高并发需求，请先让机器人进游戏预备）
    YQThread	main_timer_thread_;
};
