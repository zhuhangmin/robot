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
    // ҵ��������
    void ThreadMainProc();

private:
    //TODO NEED A LOGIC LOCK ?
    //������ �߳�
    YQThread	main_timer_thread_;
};
