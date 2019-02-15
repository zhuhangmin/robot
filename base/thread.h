// Copyright 2019 YQ

#ifndef YQBASE_THREAD_H
#define YQBASE_THREAD_H

#include <functional>
#include <memory>
#include <thread>
#include <string>

#include "noncopyable.h"

namespace YQ {

class Thread : NonCopyable {
public:
    static uint64_t GetCurrentThreadID() {
        std::hash<std::thread::id> hasher;
        return hasher(std::this_thread::get_id());
    }

public:
    typedef std::function<void()> ThreadFunc;
    Thread() {};

    explicit Thread(ThreadFunc func, const std::string& name = std::string());
    ~Thread();

    void Start();
    void Join();

    bool IsStarted() const;
    uint64_t GetThreadID();

    //#ifdef _WIN32
    //    uint32_t GetWindowThreadID();
    //#endif

private:
    bool    started_ = false;
    bool    joined_ = false;
    std::thread    thread_;
    ThreadFunc    func_;
    std::string    name_;
};

using ThreadPtr = std::unique_ptr<YQ::Thread>;
}  // namespace YQ

#endif  // YQBASE_THREAD_H
