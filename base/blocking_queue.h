// Copyright 2019 YQ

#ifndef YQBASE_BLOCKING_QUEUE_H
#define YQBASE_BLOCKING_QUEUE_H

#include <memory>
#include <string>
#include <deque>
#include <assert.h>
#include <mutex>

#include "noncopyable.h"
#include "condition.h"


namespace YQ {

template  <typename T>
class BlockingQueue : NonCopyable {
public:
    BlockingQueue() {}

    void Put(const T& x) {
        std::unique_lock<std::mutex> lk(mutex_);
        queue_.push_back(x);
        not_empty_.Notify();
    }

    void Put(T&& x) {
        std::unique_lock<std::mutex> lk(mutex_);
        queue_.push_back(std::move(x));
        not_empty_.Notify();
    }

    T Take() {
        std::unique_lock<std::mutex> lk(mutex_);
        while (queue_.empty()) {
            not_empty_.Wait(lk);
        }
        assert(!queue_.empty());
        T front(std::move(queue_.front()));
        queue_.pop_front();
        return std::move(front);
    }

    size_t Size() const {
        std::unique_lock<std::mutex> lk(mutex_);
        return queue_.size();
    }

private:
    mutable std::mutex mutex_;
    std::deque<T> queue_;
    Condition not_empty_;
};
template <typename T>
using BlockingQueuePtr = std::shared_ptr<BlockingQueue<T>>;

}  // namespace YQ

#endif  // YQBASE_BLOCKING_QUEUE_H
