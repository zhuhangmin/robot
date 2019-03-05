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
    int RobotProcess(UserID userid, RoomID roomid);

private:
    // ��������

    // ���ѡһ��û�н�����Ϸ��userid
    int GetRandomUserIDNotInGame(UserID& random_userid) const;

    // ��÷����ʱ��Ҫ�Ļ���������
    int GetRoomNeedCountMap(RoomNeedCountMap& room_count_map);

    // ����ʱ���岹��
    int DepositGainAll();

private:
    // @zhuhangmin 20190228
    // ע�⣺ ������ ���߳�
    // ��ҵ����, ���и߲������󣬿����û�����Ԥ�Ƚ���Ϸ�ȴ�
    // ����������ݲ�data, ����������������д�����ݲ�data, ֻ����д
    YQThread	main_timer_thread_;
};
