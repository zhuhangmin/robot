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
    int GetRandomUserIDNotInGame(UserID& random_userid);

    // �����Ϸҵ���㷨ָ���ķ�����Ҫ�Ļ�����
    int GetRoomCountMap(RoomCountMap& room_count_map);

private:
    //������ �߳� �����߳� ��ҵ��������, ���и߲������������û����˽���ϷԤ����
    YQThread	main_timer_thread_;
};
