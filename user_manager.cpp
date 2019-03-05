#include "stdafx.h"
#include "user_manager.h"
#include "robot_utils.h"

int UserManager::GetUser(UserID userid, UserPtr& user) const {
    std::lock_guard<std::mutex> users_lock(user_map_mutex_);
    CHECK_USERID(userid);
    return GetUserWithLock(userid, user);
}

const UserMap& UserManager::GetAllUsers() const {
    std::lock_guard<std::mutex> users_lock(user_map_mutex_);
    return user_map_;
}

int UserManager::DelUser(const UserID userid) {
    std::lock_guard<std::mutex> users_lock(user_map_mutex_);
    CHECK_USERID(userid);
    UserPtr user;
    if (kCommSucc != GetUser(userid, user)) {
        ASSERT_FALSE_RETURN;
    }

    if (user && user->get_user_id() != InvalidUserID) {
        user_map_.erase(userid);
    }
    return kCommSucc;
}

int UserManager::AddUser(const UserID userid, const UserPtr &user) {
    std::lock_guard<std::mutex> users_lock(user_map_mutex_);
    CHECK_USERID(userid);
    user_map_[userid] = user;

    return kCommSucc;
}

int UserManager::Reset() {
    std::lock_guard<std::mutex> users_lock(user_map_mutex_);
    user_map_.clear();
    return kCommSucc;
}

int UserManager::GetUserWithLock(const UserID userid, UserPtr& user) const {
    CHECK_USERID(userid);
    auto itr = user_map_.find(userid);
    if (itr == user_map_.end()) {
        return kCommFaild;
    }
    user = itr->second;
    return kCommSucc;
}

const UserFilterMap UserManager::GetAllEnterUserID() const {
    std::lock_guard<std::mutex> users_lock(user_map_mutex_);
    UserFilterMap filter_map;
    for (auto& kv : user_map_) {
        auto userid = kv.first;
        filter_map[userid] = userid;
    }
    return filter_map;
}


int UserManager::GetUserCountInRoom(const RoomID roomid, int& count) const {
    std::lock_guard<std::mutex> users_lock(user_map_mutex_);
    for (auto& kv : user_map_) {
        auto user = kv.second;
        if (user->get_room_id() == roomid) {
            count++;
        }
    }
    return kCommSucc;
}

int UserManager::GetRobotCountInRoom(const RoomID roomid, int& count) const {
    std::lock_guard<std::mutex> users_lock(user_map_mutex_);
    for (auto& kv : user_map_) {
        auto user = kv.second;
        if (user->get_room_id() == roomid) {
            if (user->get_user_type() == kUserRobot) {
                count++;
            }
        }
    }
    return kCommSucc;
}

int UserManager::GetNormalUserCountInRoom(const RoomID roomid, int& count) const {
    std::lock_guard<std::mutex> users_lock(user_map_mutex_);
    for (auto& kv : user_map_) {
        auto user = kv.second;
        if (user->get_room_id() == roomid) {
            if (user->get_user_type() == kUserNormal) {
                count++;
            }
        }
    }
    return kCommSucc;
}

int UserManager::SnapShotObjectStatus() {
    std::lock_guard<std::mutex> users_lock(user_map_mutex_);
    LOG_INFO("OBJECT ADDRESS [%x]", this);
    LOG_INFO("users_ size [%d]", user_map_.size());

    for (auto& kv : user_map_) {
        auto userid = kv.first;
        auto user = kv.second;
        LOG_INFO("userid [%d]", userid);
        user->SnapShotObjectStatus();
    }

    return kCommSucc;
}


int UserManager::SnapShotUser(UserID userid) {
    std::lock_guard<std::mutex> users_lock(user_map_mutex_);
    UserPtr user;
    if (kCommSucc != GetUserWithLock(userid, user)) {
        return kCommFaild;
    }

    user->SnapShotObjectStatus();

    return kCommSucc;
}

