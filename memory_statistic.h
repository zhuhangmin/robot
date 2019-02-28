#pragma once
#include "robot_define.h"
class MemoryStatistic : public ISingletion<MemoryStatistic> {
protected:
    SINGLETION_CONSTRUCTOR(MemoryStatistic);

public:
    void SnapShot();

private:
    std::mutex mutex_;
};

#define MEMORY_SNAPSHOT()  MemoryStatistic::Instance().SnapShot();