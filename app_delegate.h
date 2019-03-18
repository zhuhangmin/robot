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
    // ����ʱ Ϊ������Ϸ�еĻ����� ����������������
    int ConnectHallForAllRobot();

    // ����ʱ ������½
    int LogonHall(const UserID& userid) const;

    // ����ʱ ������ȡ�û���Ϣ
    int GetUserGameInfo(const UserID& userid) const;

    // ����ʱ Ϊ������Ϸ�еĻ����� ����������Ϸ����
    int ConnectGameForRobotInGame();

    // ���ˣ����δ����+���ӵȴ� ��ʵ��Ҽ���
    int GetWaittingUser(UserMap& filter_user_map) const;

    // 0 ���ѡһ�� 1���Ϸ��� 2�������� 3û������Ϸ 4���쳣
    int GetRandomQualifiedRobotUserID(const RoomID& roomid, const TableNO& tableno, UserID& random_userid) const;

    // ��ô˷�����Ҫ������ƥ����û���Ϣ
    int GetRoomNeedUserMap(UserMap& need_user_map);

    // ������Ϸ
    int EnterGame(const UserID& userid, const RoomID& roomid, const TableNO& tableno);

    // ����: ��������Ļ�����
    int FilterRobotNotInTableDepositRange(const RoomID& roomid, const TableNO& tableno, RobotUserIDMap& not_logon_game_temp) const;

    // ����: ���Ϸ�������Ļ�����
    int FilterRobotNotInRoomDepositRange(RobotUserIDMap& not_logon_game_temp) const;

public:
#ifdef _DEBUG
    //TEST ONLY
    void SendTestRobot(const UserID& userid, const RoomID& roomid, const TableNO& tableno);

    int TestLogonHall(const UserID& userid) const;
#endif

private:
    // ������ ���߳� ��ҵ����
    // ����������ݲ�data, ����������������д�����ݲ�data, ֻ����д
    YQThread	timer_thread_;

    RobotUserIDMap robot_in_other_game_map_;
};
