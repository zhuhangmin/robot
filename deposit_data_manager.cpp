#include "stdafx.h"
#include "deposit_data_manager.h"
#include "robot_utils.h"

int DepositDataManager::Init() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO_FUNC("\t[START]");
    return kCommSucc;
}

int DepositDataManager::Term() {
    std::lock_guard<std::mutex> lock(mutex_);
    user_game_info_map_.clear();
    LOG_INFO_FUNC("[EXIT]");
    return kCommSucc;
}

int DepositDataManager::SetUserGameData(const int& userid, USER_GAMEINFO_MB* info) {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_USERID(userid);
    user_game_info_map_[userid] = *info;
    return kCommSucc;
}

UserGameInfoMap DepositDataManager::GetUserGameDataMap() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return user_game_info_map_;
}


int DepositDataManager::GetDeposit(const int& userid, int64_t& deposit) {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_USERID(userid);
    if (kCommSucc != GetDepositWithLock(userid, deposit)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int DepositDataManager::SetDeposit(const int& userid, const int64_t& deposit) {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_USERID(userid);
    CHECK_DEPOSIT(deposit);
    if (kCommSucc != SetDepositWithLock(userid, deposit)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int DepositDataManager::AddDeposit(const int& userid, const int64_t& amount) {
    std::lock_guard<std::mutex> lock(mutex_);
    int64_t old_deposit = 0;
    if (kCommSucc != GetDepositWithLock(userid, old_deposit)) {
        ASSERT_FALSE_RETURN;
    }

    int64_t new_deposit = old_deposit + amount;
    CHECK_DEPOSIT(new_deposit);

    if (kCommSucc != SetDepositWithLock(userid, new_deposit)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int DepositDataManager::GetDepositWithLock(const int& userid, int64_t& deposit) {
    CHECK_USERID(userid);
    const auto iter = user_game_info_map_.find(userid);
    if (iter != user_game_info_map_.end()) {
        deposit = user_game_info_map_[userid].nDeposit;
        return kCommSucc;
    }
    return kCommFaild;
}

int DepositDataManager::SetDepositWithLock(const int& userid, const int64_t& deposit) {
    CHECK_USERID(userid);
    CHECK_DEPOSIT(deposit);
    const auto iter = user_game_info_map_.find(userid);
    if (iter != user_game_info_map_.end()) {
        user_game_info_map_[userid].nDeposit = deposit;
        LOG_INFO("[DEPOSIT] userid [%d] deposit [%I64d]", userid, deposit);
        return kCommSucc;
    }
    return kCommFaild;
}

int DepositDataManager::SnapShotObjectStatus() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("user_game_info_map_ size [%d]", user_game_info_map_.size());
    for (auto& kv : user_game_info_map_) {
        const auto userid = kv.first;
        const auto game_info = kv.second;
        LOG_INFO("robot userid [%d] deposit [%I64d]", userid, game_info.nDeposit);
    }

    return kCommSucc;
}