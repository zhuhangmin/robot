#pragma once
#include "robot_define.h"
#include "user_manager.h"

// ��Ϸҵ��
class AppDelegate {
public:
    int Init();

    int Term();

protected:
    int InitLanuch() const;

private:
    // ҵ���̺߳��� 
    int ThreadMainProc();

    // ҵ��������
    int MainProcess();

    // ���������� ������� ������Ϸ
    int RobotProcess(const UserID& userid, const RoomID& roomid, const TableNO& tableno);

private:
    // ��������

    // ���ѡһ��û�н�����Ϸ��userid
    int GetRandomUserIDNotInGame(UserID& random_userid) const;

    // ��ô˷�����Ҫ������ƥ����û���Ϣ
    int GetRoomNeedUserMap(UserMap& need_user_map);

    // ����ʱ ���岹��
    int DepositGainAll();

    // ����ʱ Ϊ������Ϸ�еĻ����� ����������������
    int ConnectHallForAllRobot();

    // ����ʱ Ϊ������Ϸ�еĻ����� ����������Ϸ����
    int ConnectGameForRobotInGame();

    // ��½����
    int LogonHall(const UserID& userid);

    // ������Ϸ
    int EnterGame(const UserID& userid, const RoomID& roomid, const TableNO& tableno);

    // ���ˣ����δ����+���ӵȴ� ��ʵ��Ҽ���
    int GetWaittingUser(UserMap& filter_user_map);

public:
    //TEST ONLY
    void SendTestRobot(const UserID& userid, const RoomID& roomid, const TableNO& tableno);

    //TEST ONLY
    int TestLogonHall(const UserID& userid);

private:
    // ������ ���߳� ��ҵ����
    // ����������ݲ�data, ����������������д�����ݲ�data, ֻ����д
    YQThread	main_timer_thread_;

    bool inited_{false};
};
