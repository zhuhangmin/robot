#pragma once


class noncopyable {
public:
    noncopyable(const noncopyable&) = delete;
    void operator=(const noncopyable&) = delete;
    noncopyable() = default;
    ~noncopyable() = default;
};

