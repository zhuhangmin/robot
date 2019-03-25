#pragma once
#include "robot_define.h"
class RobotStatistic : public ISingletion<RobotStatistic> {
public:
    // 埋点
    int Event(const EventType& type, const EventKey& key);

    int SnapShotObjectStatus();

    // 运行时信息
    int ProcessStatus();
protected:
    SINGLETION_CONSTRUCTOR(RobotStatistic);

private:
    // 发送消息统计
    SendMsgCountMap send_msg_count_map;

    // 通知消息统计
    RecvNtfCountMap recv_msg_count_map;

    // 消息错误统计
    ErrorCountMap error_msg_count_map;
};


#define EVENT_TRACK(type, key)  RobotStatistic::Instance().Event(type, key)


