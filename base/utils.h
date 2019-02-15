// Copyright 2019 YQ

#ifndef YQBASE_UTILS_H
#define YQBASE_UTILS_H

#include <chrono>
#include "Def.h"
#include <random>
#include <ctime>
#include <map>
#include <chrono>
#include <assert.h>
#include <iostream>

namespace YQ {
inline void Sleep(int seconds) {
    std::chrono::seconds dura(seconds);
    std::this_thread::sleep_for(dura);
}

inline void SleepMS(int milli_seconds) {
    std::chrono::milliseconds dura(milli_seconds);
    std::this_thread::sleep_for(dura);
}


inline ResultCode RandomBetweenRange(int min_value, int max_value, int& reuslt) {
    if (max_value < min_value) {
        assert(false);
        return ResultCode::kFailed;
    }
    static std::default_random_engine engine((unsigned int) (std::time(nullptr)));
    auto random_result = engine();
    reuslt = random_result % (max_value - min_value + 1) + min_value;
    return ResultCode::kSuccess;
}

//NOTE NOT THREAD SAFE
//TEST ONLY
static std::map<std::string, std::chrono::system_clock::time_point> ProfileMap;

inline void ProfileBeg(const std::string& key) {
    auto beg = std::chrono::system_clock::now();
    ProfileMap[key] = beg;
}

inline void ProfileEnd(const std::string& key) {
    auto end = std::chrono::system_clock::now();
    if (ProfileMap.find(key) != ProfileMap.end()) {
        auto beg = ProfileMap[key];
        printf("%s cost %d ms \n", key.c_str(), std::chrono::duration_cast<std::chrono::milliseconds>(end - beg).count());
    }
}

typedef std::chrono::system_clock::time_point TimeStamp;

inline int64_t GetCurrentTimeStampMS() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

inline int64_t GetCurrentTimeStampNano() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

inline int64_t TimeDifference(int64_t beg, int64_t end) {
    return end - beg;
}

}//  namespace YQ


#endif  // YQBASE_UTILS_H