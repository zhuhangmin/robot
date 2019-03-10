#include "stdafx.h"
#include "robot_utils.h"
#include "robot_statistic.h"

void RobotStatistic::Event(EventType type, EventKey key) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (type == EventType::kSend) {
        send_msg_count_map[key]++;
    }

    if (type == EventType::kRecv) {
        recv_msg_count_map[key]++;
    }

    if (type == EventType::kErr) {
        error_msg_count_map[key]++;
    }

}

void  RobotStatistic::SnapShotObjectStatus() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("SEND");
    for (auto& kv : send_msg_count_map) {
        LOG_INFO("count [%I64d] [%s %d]",
                 kv.second, REQ_STR(kv.first), kv.first);
    }

    LOG_INFO("RECV");
    for (auto& kv : recv_msg_count_map) {
        LOG_INFO("count [%I64d] [%s %d]",
                 kv.second, REQ_STR(kv.first), kv.first);
    }

    LOG_INFO("ERROR");
    for (auto& kv : error_msg_count_map) {
        LOG_INFO("count [%d] [%s %d]",
                 kv.second, ERR_STR(kv.first), kv.first);
    }
}
