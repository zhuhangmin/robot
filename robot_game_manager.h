#pragma once
#include "robot_define.h"
#include "robot_game.h"

// �����˹�����
class RobotGameManager : public ISingletion<RobotGameManager> {
public:
    using RobotMap = std::unordered_map<UserID, RobotPtr>;

public:
    int Init();

    int Term();

public:
    // ��ã������ڴ�����ָ��������
    int GetRobotWithCreate(UserID userid, RobotPtr& robot);

    // ������л����˽�����Ϣ���߳�
    ThreadID GetRobotNotifyThreadID();

protected:
    SINGLETION_CONSTRUCTOR(RobotGameManager);

private:
    // ������ ��ʱ����
    int ThreadRobotPluse();

    // ������ ��Ϣ����
    int ThreadRobotNotify();

    // ������ ��Ϣ����
    int OnRobotNotify(RequestID requestid, void* ntf_data_ptr, int data_size, TokenID token_id);

    // ������ ��������
    int SendGamePluse();

private:
    //�������� WithLock ��ʶ����ǰ��Ҫ��ô˶����������mutex

    int GetRobotWithLock(UserID userid, RobotPtr& robot);

    int SetRobotWithLock(RobotPtr robot);

    int GetRobotByTokenWithLock(const TokenID token_id, RobotPtr& robot);

private:
    //������ ������
    std::mutex robot_map_mutex_;

    //������ ��Ϸ���������Ӽ���
    RobotMap robot_map_;

    //������ ������Ϣ�߳�
    YQThread robot_notify_thread_;

    //������ �����߳�
    YQThread robot_heart_timer_thread_;

};

#define RobotMgr RobotGameManager::Instance()