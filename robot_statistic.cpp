#include "stdafx.h"
#include "robot_statistic.h"

int RobotStatistic::Event(const EventType& type, const EventKey& key) {
    return kCommSucc;
}

int RobotStatistic::SnapShotObjectStatus() {
    return kCommSucc;
}

int RobotStatistic::ProcessStatus() {
    return kCommSucc;
}