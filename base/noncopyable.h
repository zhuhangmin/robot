// Copyright 2019 YQ

#ifndef YQBASE_NONCOPYABLE_H
#define YQBASE_NONCOPYABLE_H

namespace YQ {

class NonCopyable {
public:
    NonCopyable(const NonCopyable&) = delete;
    void operator=(const NonCopyable&) = delete;

protected:
    NonCopyable() = default;
    ~NonCopyable() = default;
};

}  // namespace YQ

#endif  // YQBASE_NONCOPYABLE_H

