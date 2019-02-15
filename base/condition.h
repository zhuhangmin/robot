// Copyright 2019 YQ

#ifndef YQBASE_CONDITION_H
#define YQBASE_CONDITION_H

#include <memory>
#include <condition_variable>

#include "noncopyable.h"
#include "def.h"

namespace YQ {

class Condition : NonCopyable {

public:
    Condition() {};

    ~Condition() {};

    void Wait(UniqueLock& lk) {
        pcond_.wait(lk);
    }

    // returns true if time out, false otherwise.
    // guard by lock
    bool WaitSeconds(UniqueLock& lk, unsigned int seconds);

    //guard by lock
    bool WaitMS(UniqueLock& lk, uint64_t million_seconds);

    void Notify() {
        pcond_.notify_one();
    }

    void NotifyAll() {
        pcond_.notify_all();
    }

private:
    std::condition_variable pcond_;
};

}  // namespace YQ

#endif  // YQBASE_CONDITION_H
