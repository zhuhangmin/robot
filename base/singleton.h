// Copyright 2019 YQ

#ifndef YQBASE_SINGLETON_H
#define YQBASE_SINGLETON_H

#include <mutex>
#include "noncopyable.h"

namespace YQ {

template  <typename T>
class Singleton : NonCopyable {
public:
    static T& GetInstance(){
    std::call_once(once_flag_,[]{
		instance_ = new T();
        });

    return *instance_;
}
    
private:
    static T* instance_;
    static std::once_flag once_flag_;

};

template<typename T>
T* Singleton<T>::instance_ = nullptr;

template<typename T>
std::once_flag Singleton<T>::once_flag_;


}  // namespace YQ

#endif  // YQBASE_SINGLETON_H
