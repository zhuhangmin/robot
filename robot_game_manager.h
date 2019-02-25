#pragma once
#include "robot_define.h"
#include "robot_game.h"

// �����˹�����
class RobotGameManager : public ISingletion<RobotGameManager> {
public:
    using RobotMap = std::unordered_map<UserID, RobotPtr>;

public:
    int Init();

    void Term();

public:
    RobotPtr GetRobotWithCreate(UserID userid);

    ThreadID GetRobotNotifyThreadID();
protected:
    SINGLETION_CONSTRUCTOR(RobotGameManager);

private:
    // ������ ��ʱ����
    void ThreadRobotPluse();

    // ������ ��Ϣ����
    void ThreadRobotNotify();

    // ������ ��Ϣ����
    int OnRobotNotify(RequestID requestid, void* ntf_data_ptr, int data_size, TokenID token_id);

private:
    //����ҵ��

    // ��Ϸ ����
    void SendGamePluse();

    // ������
    RobotPtr GetRobotWithLock(UserID userid);
    void SetRobotWithLock(RobotPtr robot);
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