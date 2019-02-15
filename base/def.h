// Copyright 2019 YQ

#ifndef YQBASE_DEF_H
#define YQBASE_DEF_H

#include <mutex>

namespace YQ {

const int MaxThreadTask = 1000;

const int kMaxTaskQueue = 10000;

typedef std::unique_lock<std::mutex> UniqueLock;

enum class ResultCode {
    kFailed = -1,
    kSuccess = 0,
};

}  // namespace YQ

#endif  // YQBASE_DEF_H