#include "stdafx.h"
#include "UserMgr.h"

std::shared_ptr<User> UserMgr::NewUser() {
    return std::make_shared<User>();
}

std::shared_ptr<User> UserMgr::GetUser(int userid) {
    static std::shared_ptr<User> null_user = std::make_shared<User>();

    std::lock_guard<std::mutex> users_lock(users_mutex);
    auto itr = users_.find(userid);
    if (itr == users_.end()) {
        return null_user;
    }
    return itr->second;
}

std::hash_map<int, std::shared_ptr<User>> UserMgr::GetAllUsers() {
    std::lock_guard<std::mutex> users_lock(users_mutex);
    return users_;
}

void UserMgr::DelUser(int userid) {
    // 删除用户之前先把用户的id置为0，防止erase后还有shared_ptr在拿着原来的数据使用
    std::lock_guard<std::mutex> users_lock(users_mutex);
    std::shared_ptr<User> user = GetUser(userid);
    {
        user->set_user_id(0);
    }
    users_.erase(userid);
}

void UserMgr::AddUser(int userid, const std::shared_ptr<User> &user) {
    std::lock_guard<std::mutex> users_lock(users_mutex);
    users_[userid] = user;
}

bool UserMgr::IsValidUserID(int userid) {
    return userid > 0;
}

void UserMgr::Reset() {
    users_.clear();
}
