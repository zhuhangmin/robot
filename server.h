#pragma once
#include "robot_define.h"

class CMainServer {
public:
    CMainServer();
    ~CMainServer();

public:
    int Init();

    void Term();

protected:
    int InitLanuch();

private:
    // 业务主流程
    void ThreadMainProc();

private:
    //TODO NEED A LOGIC LOCK ?
    //主流程 线程
    YQThread	main_timer_thread_;
};
