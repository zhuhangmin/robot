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
    // ҵ��������
    int ThreadMainProc();

private:
    // ���ѡһ��û�н�����Ϸ��userid
    int FindRandomUserIDNotInGame(UserID& random_userid);

private:
    //������ �߳� �����߳� ��ҵ��������, ���и߲������������û����˽���ϷԤ����
    YQThread	main_timer_thread_;
};
