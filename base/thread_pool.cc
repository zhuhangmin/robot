#include "stdafx.h"

#include "thread_pool.h"

#include <type_traits>
#include <assert.h>
#include <errno.h>
#include <stdio.h>

namespace YQ {
ThreadPool::ThreadPool(const std::string& nameArg /*= std::string("ThreadPool")*/)
    :name_(nameArg) {

}

ThreadPool::~ThreadPool() {
    if (running_) {
        stop();
    }
}

void ThreadPool::start(int numThreads) {
    assert(threads_.empty());
    running_ = true;
    threads_.reserve(numThreads);

    for (int i = 0; i < numThreads; ++i) {
        char id[32];
        _snprintf_s(id, sizeof(id), "%d", i + 1);
        threads_.emplace_back(new YQ::Thread(
            std::bind(&ThreadPool::runInThread, this),
            name_ + id));
        threads_[i]->Start();
    }

    if (numThreads == 0 && threadInitCallback_) {
        threadInitCallback_();
    }
}

void ThreadPool::stop() {
    {
        std::lock_guard<std::mutex> lk(mutex_);
        running_ = false;
        not_empty_.NotifyAll();
    }
    for (auto& thr : threads_) {
        thr->Join();
    }
}

size_t ThreadPool::queueSize() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return task_queue_.size();
}

void ThreadPool::run(Task task) {
    if (threads_.empty()) {
        task();
    } else {
        std::unique_lock<std::mutex> lk(mutex_);
        while (isFull()) {
            not_full_.Wait(lk);
        }
        assert(!isFull());

        task_queue_.push_back(std::move(task));
        not_empty_.Notify();
    }
}

ThreadPool::Task ThreadPool::take() {
    std::unique_lock<std::mutex> lk(mutex_);
    // always use a while-loop, due to spurious wakeup
    if (task_queue_.empty() && running_) {
        not_empty_.Wait(lk);
    }

    Task task;
    if (!task_queue_.empty()) {
        task = task_queue_.front();
        task_queue_.pop_front();
        if (maxQueueSize_ > 0) {
            not_full_.Notify();
        }
    }

    return task;
}

bool ThreadPool::isFull() const {
    //is lock by this thread
    return maxQueueSize_ > 0 && task_queue_.size() >= maxQueueSize_;
}

void ThreadPool::runInThread() {
    try {
        if (threadInitCallback_) {
            threadInitCallback_();
        }
        while (running_) {
            Task task(take()); //?
            if (task) {
                task();
            }
        }
    } catch (const std::exception& ex) {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    } catch (...) {
        fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
        throw; // rethrow
    }
}

void ThreadPool::setMaxQueueSize(int maxSize) {
    maxQueueSize_ = maxSize;
}

void ThreadPool::setThreadInitCallback(const Task& cb) {
    threadInitCallback_ = cb;
}

const std::string& ThreadPool::name() const {
    return name_;
}






}  // namespace YQ
