#include "stdafx.h"
#include "user_manager.h"
#include "robot_utils.h"

int UserManager::GetUser(const UserID& userid, UserPtr& user) const {
    std::lock_guard<std::mutex> users_lock(mutex_);
    CHECK_USERID(userid);
    return GetUserWithLock(userid, user);
}

UserMap UserManager::GetAllUsers() const {
    std::lock_guard<std::mutex> users_lock(mutex_);
    return user_map_;
}

int UserManager::DelUser(const UserID& userid) {
    std::lock_guard<std::mutex> users_lock(mutex_);
    CHECK_USERID(userid);
    UserPtr user;
    if (kCommSucc != GetUserWithLock(userid, user)) {
        ASSERT_FALSE_RETURN;
    }

    if (user && user->get_user_id() != InvalidUserID) {
        user_map_.erase(userid);
    }
    return kCommSucc;
}

int UserManager::AddUser(const UserID& userid, const UserPtr &user) {
    std::lock_guard<std::mutex> users_lock(mutex_);
    CHECK_USERID(userid);
    if (kCommSucc != AddUserWithLock(userid, user)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int UserManager::Reset() {
    std::lock_guard<std::mutex> users_lock(mutex_);
    user_map_.clear();
    return kCommSucc;
}

int UserManager::ResetDataAndReInit(const game::base::GetGameUsersResp& resp) {
    LOG_WARN("RESET Data UserManger And ReInit");
    std::lock_guard<std::mutex> users_lock(mutex_);
    user_map_.clear();
    for (auto user_index = 0; user_index < resp.users_size(); user_index++) {
        if (kCommSucc != AddUserPBWithLock(resp.users(user_index))) {
            ASSERT_FALSE;
            continue;
        }
    }
    return kCommSucc;
}

int UserManager::AddUserPBWithLock(const game::base::User& user_pb) const {
    auto user = std::make_shared<User>();
    user->set_user_id(user_pb.userid());
    user->set_room_id(user_pb.roomid());
    user->set_table_no(user_pb.tableno());
    user->set_chair_no(user_pb.chairno());
    user->set_user_type(user_pb.user_type());
    user->set_deposit(user_pb.deposit());
    user->set_total_bout(user_pb.total_bout());
    user->set_offline_count(user_pb.offline_count());
    if (kCommSucc != UserMgr.AddUserWithLock(user->get_user_id(), user)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int UserManager::AddUserWithLock(const UserID& userid, const UserPtr &user) {
    CHECK_USERID(userid);
    user_map_[userid] = user;
    return kCommSucc;
}

int UserManager::GetUserWithLock(const UserID& userid, UserPtr& user) const {
    CHECK_USERID(userid);
    const auto itr = user_map_.find(userid);
    if (itr == user_map_.end()) {
        LOG_ERROR("userid [%d] not exist in user manager", userid);
        ASSERT_FALSE_RETURN;
    }
    user = itr->second;
    return kCommSucc;
}

UserFilterMap UserManager::GetAllEnterUserID() const {
    std::lock_guard<std::mutex> users_lock(mutex_);
    UserFilterMap filter_map;
    for (auto& kv : user_map_) {
        auto userid = kv.first;
        filter_map[userid] = userid;
    }
    return filter_map;
}


int UserManager::GetUserCountInRoom(const RoomID& roomid, int& count) const {
    std::lock_guard<std::mutex> users_lock(mutex_);
    for (auto& kv : user_map_) {
        const auto user = kv.second;
        if (user->get_room_id() == roomid) {
            count++;
        }
    }
    return kCommSucc;
}

int UserManager::GetRobotCountInRoom(const RoomID& roomid, int& count) const {
    std::lock_guard<std::mutex> users_lock(mutex_);
    for (auto& kv : user_map_) {
        const auto user = kv.second;
        if (user->get_room_id() == roomid) {
            if (IS_BIT_SET(user->get_user_type(), kUserRobot)) {
                count++;
            }
        }
    }
    return kCommSucc;
}

int UserManager::GetNormalUserCountInRoom(const RoomID& roomid, int& count) const {
    std::lock_guard<std::mutex> users_lock(mutex_);
    for (auto& kv : user_map_) {
        const auto user = kv.second;
        if (user->get_room_id() == roomid) {
            if (user->get_user_type() == kUserNormal) {
                count++;
            }
        }
    }
    return kCommSucc;
}


int UserManager::GetNormalUserMap(UserMap& normal_user_map) const {
    std::lock_guard<std::mutex> users_lock(mutex_);
    for (auto& kv : user_map_) {
        const auto user = kv.second;
        const auto user_tpye = user->get_user_type();
        if (!IS_BIT_SET(user_tpye, kUserRobot)) {
            normal_user_map[kv.first] = kv.second;
        }
    }
    return kCommSucc;
}

int UserManager::GetRobotUserMap(UserMap& robot_user_map) const {
    std::lock_guard<std::mutex> users_lock(mutex_);
    for (auto& kv : user_map_) {
        const auto user = kv.second;
        if (user->get_user_type() == kUserRobot) {
            robot_user_map[kv.first] = kv.second;
        }
    }
    return kCommSucc;
}

bool UserManager::IsRobotUserExist(const UserID& userid) const {
    std::lock_guard<std::mutex> users_lock(mutex_);
    const auto iter = user_map_.find(userid);
    if (iter != user_map_.end()) {
        const auto user_ptr = iter->second;
        if (user_ptr->get_user_type() == kUserRobot) {
            return true;
        }
    }
    return false;
}

int UserManager::SnapShotObjectStatus() {
    std::lock_guard<std::mutex> users_lock(mutex_);
    LOG_INFO("OBJECT ADDRESS [%x]", this);
    LOG_INFO("users_ size [%d]", user_map_.size());

    for (auto& kv : user_map_) {
        auto user = kv.second;
        user->SnapShotObjectStatus();
    }

    return kCommSucc;
}

int UserManager::SnapShotUser(const UserID& userid) const {
    std::lock_guard<std::mutex> users_lock(mutex_);
    UserPtr user;
    if (kCommSucc != GetUserWithLock(userid, user)) {
        ASSERT_FALSE_RETURN;
    }

    user->SnapShotObjectStatus();

    return kCommSucc;
}

int UserManager::BriefInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("users_ size [%d]", user_map_.size());
    std::string str = "{";
    for (auto& kv : user_map_) {
        const auto userid = kv.first;
        str += "userid ";
        str += "[";
        str += std::to_string(userid);
        str += "] ";
    }
    str += "}";
    LOG_INFO(" [%s]", str.c_str());
    return kCommSucc;
}


