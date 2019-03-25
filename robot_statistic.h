#pragma once
#include "robot_define.h"
class RobotStatistic : public ISingletion<RobotStatistic> {
public:
    // ���
    int Event(const EventType& type, const EventKey& key);

    int SnapShotObjectStatus();

    // ����ʱ��Ϣ
    int ProcessStatus();
protected:
    SINGLETION_CONSTRUCTOR(RobotStatistic);

private:
    // ������Ϣͳ��
    SendMsgCountMap send_msg_count_map;

    // ֪ͨ��Ϣͳ��
    RecvNtfCountMap recv_msg_count_map;

    // ��Ϣ����ͳ��
    ErrorCountMap error_msg_count_map;
};


#define EVENT_TRACK(type, key)  RobotStatistic::Instance().Event(type, key)


