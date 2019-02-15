// Copyright 2019 YQ

#ifndef YQBASE_THREADPOOL_H
#define YQBASE_THREADPOOL_H

#include <functional>
#include <memory>
#include <thread>
#include <deque>
#include <queue>
#include <string>

#include "noncopyable.h"
#include "def.h"
#include "thread.h"
#include "condition.h"




namespace YQ {
class ThreadPool : NonCopyable {
public:
    typedef std::function<void()>	Task;
    typedef std::deque<Task>		TaskQueue;
    typedef std::vector<std::unique_ptr<YQ::Thread>> ThreadVec;

public:
    explicit ThreadPool(const std::string& nameArg = std::string("ThreadPool"));
    ~ThreadPool();

    // Must be called before start().
    void setMaxQueueSize(int maxSize);
    void setThreadInitCallback(const Task& cb);

    void start(int numThreads);
    void stop();

    const std::string& name() const;
    size_t queueSize() const;

    // Could block if maxQueueSize > 0
    void run(Task task);

private:
    bool isFull() const; // guard by mutex
    void runInThread();
    Task take();

    mutable std::mutex mutex_; //?

    Condition not_empty_; // guard by mutex
    Condition not_full_; // guard by mutex
    std::string name_;
    Task threadInitCallback_; //?
    ThreadVec threads_;
    TaskQueue task_queue_; // guard by mutex
    size_t maxQueueSize_ = 0;
    bool running_ = false;
};

}  // namespace YQ

#endif  // YQBASE_THREADPOOL_H
