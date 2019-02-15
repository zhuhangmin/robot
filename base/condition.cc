// Copyright 2019 YQ
#include "stdafx.h"
#include <chrono>

#include "condition.h"

namespace YQ {

bool YQ::Condition::WaitSeconds(UniqueLock& lk, unsigned int seconds) {
    uint64_t million_seconds = (uint64_t) (seconds * 1000);
    return WaitMS(lk, million_seconds);
}


bool YQ::Condition::WaitMS(UniqueLock& lk, uint64_t million_seconds) {
    std::chrono::milliseconds dura(million_seconds);
    auto cv_status = pcond_.wait_for(lk, dura);
    return (cv_status == std::cv_status::timeout);
}

}  // namespace YQ
