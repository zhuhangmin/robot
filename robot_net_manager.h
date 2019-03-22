#pragma once
#include "robot_define.h"
#include "robot_net.h"

using RobotMap = std::unordered_map<UserID, RobotPtr>;

class RobotNetManager : public ISingletion<RobotNetManager> {
public:
    int Init();

    int Term();

    // ������ ���� ���ͽ�����Ϸ
    int EnterGame(const UserID& userid, const RoomID& roomid, const TableNO& tableno, std::string game_ip, int game_port);

private:
    // ������ ��ʱ��Ϣ
    int ThreadTimer();

    // ������ ��Ϣ����
    int ThreadNotify();

    // ������ ��Ϣ����
    int OnNotify(const RequestID& requestid, const REQUEST& request, const TokenID& token_id);

    // ������ ��������
    int SendPulse();

    // ������ ���ӱ���
    int KeepConnection();

private:
    //�������� WithLock ��ʶ����ǰ��Ҫ��ô˶����������mutex

    int GetRobotWithLock(const UserID& userid, RobotPtr& robot) const;

    int SetRobotWithLock(const RobotPtr& robot);

    int GetRobotByTokenWithLock(const TokenID& token_id, RobotPtr& robot) const;

public:
    // ����״̬����
    int SnapShotObjectStatus();

    int BriefInfo() const;

protected:
    SINGLETION_CONSTRUCTOR(RobotNetManager);

private:
    //������ ������
    mutable std::mutex mutex_;

    //������ ��Ϸ���������Ӽ��� (ֻ������)
    RobotMap robot_map_;

    //������ ������Ϣ�߳�
    YQThread notify_thread_;

    //������ �����߳�
    YQThread timer_thread_;

};

#define RobotMgr RobotNetManager::Instance()