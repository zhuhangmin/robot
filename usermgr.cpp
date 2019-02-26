#include "stdafx.h"
#include "UserMgr.h"
#include "robot_utils.h"

int UserMgr::GetUser(UserID userid, UserPtr& user) const {
    CHECK_USERID(userid);
    std::lock_guard<std::mutex> users_lock(users_mutex);
    auto itr = users_.find(userid);
    if (itr == users_.end()) {
        return kCommFaild;
    }
    user = itr->second;
    return kCommSucc;
}

const UserMap& UserMgr::GetAllUsers() const {
    std::lock_guard<std::mutex> users_lock(users_mutex);
    return users_;
}

int UserMgr::DelUser(const UserID userid) {
    CHECK_USERID(userid);
    std::lock_guard<std::mutex> users_lock(users_mutex);
    UserPtr user;
    if (kCommSucc != GetUser(userid, user)) {
        assert(false);
        return kCommFaild;
    }

    if (user && user->get_user_id() != InvalidUserID) {
        users_.erase(userid);
    }
    return kCommSucc;
}

int UserMgr::AddUser(const UserID userid, const UserPtr &user) {
    CHECK_USERID(userid);
    std::lock_guard<std::mutex> users_lock(users_mutex);
    users_[userid] = user;

    return kCommSucc;
}

int UserMgr::Reset() {
    users_.clear();
    return kCommSucc;
}
