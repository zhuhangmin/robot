#include "stdafx.h"
#include "UserMgr.h"
#include "robot_utils.h"

int UserMgr::GetUser(int userid, std::shared_ptr<User>& user) {
    CHECK_USERID(userid);
    std::lock_guard<std::mutex> users_lock(users_mutex);
    auto itr = users_.find(userid);
    if (itr == users_.end()) {
        return kCommFaild;
    }
    user = itr->second;
    return kCommSucc;
}

std::hash_map<int, std::shared_ptr<User>> UserMgr::GetAllUsers() {
    std::lock_guard<std::mutex> users_lock(users_mutex);
    return users_;
}

int UserMgr::DelUser(int userid) {
    CHECK_USERID(userid);
    std::lock_guard<std::mutex> users_lock(users_mutex);
    std::shared_ptr<User> user;
    if (kCommSucc != GetUser(userid, user)) {
        assert(false);
        return kCommFaild;
    }

    if (user && user->get_user_id() != InvalidUserID) {
        users_.erase(userid);
    }
    return kCommSucc;
}

int UserMgr::AddUser(int userid, const std::shared_ptr<User> &user) {
    CHECK_USERID(userid);
    std::lock_guard<std::mutex> users_lock(users_mutex);
    users_[userid] = user;

    return kCommSucc;
}

bool UserMgr::IsValidUserID(int userid) {
    return userid > 0;
}

void UserMgr::Reset() {
    users_.clear();
}
