// Copyright 2019 YQ
#include "stdafx.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <type_traits>
#include <system_error>

#include "thread.h"

namespace YQ {

Thread::Thread(ThreadFunc func,
               const std::string& name)
               :func_(std::move(func)),
               name_(name) {}

Thread::~Thread() {
    if (started_ && !joined_) {
        //thread_.join();
        thread_.detach(); //@zhuhangmin exe 退出时 join卡住，detach让系统回收
    }
}

void Thread::Start() {
    assert(!started_);
    started_ = true;
    thread_.swap(std::thread(func_));
}

void Thread::Join() {
    assert(started_);
    assert(!joined_);
    joined_ = true;
    thread_.join();
}

bool Thread::IsStarted() const {
    return started_;
}

uint64_t Thread::GetThreadID() {
    std::hash<std::thread::id> hasher;
    auto id = hasher(thread_.get_id());
    return id;
}

//#ifdef _WIN32
//uint32_t Thread::GetWindowThreadID() {
//    return ::GetThreadId((HANDLE) thread_.native_handle());
//}
//#endif



}  // namespace YQ
