#pragma once
#include "robot_define.h"

class MainServer {
public:
    MainServer();
    ~MainServer();

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
