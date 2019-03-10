#pragma once
#include "robot_define.h"

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
    int RobotProcess(const UserID& userid, const RoomID& roomid);

private:
    // ��������

    // ���ѡһ��û�н�����Ϸ��userid
    int GetRandomUserIDNotInGame(UserID& random_userid) const;

    // ��÷����ʱ��Ҫ�Ļ���������
    int GetRoomNeedCountMap(RoomNeedCountMap& room_count_map);

    // ����ʱ ���岹��
    int DepositGainAll();

    // ����ʱ Ϊ������Ϸ�еĻ����� ����������������
    int ConnectHallForAllRobot();

    // ����ʱ Ϊ������Ϸ�еĻ����� ����������Ϸ����
    int ConnectGameForRobotInGame();

    // ��½����
    int LogonHall(UserID userid);

    // ������Ϸ
    int EnterGame(UserID userid, RoomID roomid, TableNO tableno = InvalidTableNO);

private:
    // @zhuhangmin 20190228
    // ע�⣺ ������ ���߳�
    // ��ҵ����, ���и߲������󣬿����û�����Ԥ�Ƚ���Ϸ�ȴ�
    // ����������ݲ�data, ����������������д�����ݲ�data, ֻ����д
    YQThread	main_timer_thread_;

    bool inited_{false};
};
