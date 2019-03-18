#pragma once
#include "robot_define.h"
class RobotStatistic : public ISingletion<RobotStatistic> {
public:
    void Event(const EventType& type, const EventKey& key);

    void SnapShotObjectStatus();
protected:
    SINGLETION_CONSTRUCTOR(RobotStatistic);

private:
    std::mutex mutex_;
    SendMsgCountMap send_msg_count_map;
    RecvNtfCountMap recv_msg_count_map;
    ErrorCountMap error_msg_count_map;
};


#define EVENT_TRACK(type, key)  RobotStatistic::Instance().Event(type, key)


