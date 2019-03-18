#include "stdafx.h"
#include "room_manager.h"
#include "robot_utils.h"

int RoomManager::GetRoom(const RoomID& roomid, RoomPtr& room) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto iter = rooms_.find(roomid);
    if (iter == rooms_.end()) {
        ASSERT_FALSE_RETURN;
    }
    room = iter->second;
    return kCommSucc;
}

int RoomManager::AddRoom(const RoomID& roomid, const RoomPtr &room) {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_ROOMID(roomid);
    CHECK_ROOM(room);
    rooms_[roomid] = room;
    return kCommSucc;
}

int RoomManager::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    rooms_.clear();
    return kCommSucc;
}

int RoomManager::GetChairInfo(const RoomID& roomid, const TableNO& tableno, const int& userid, ChairInfo& info) const {
    std::lock_guard<std::mutex> lock(mutex_);
    TablePtr table;
    if (kCommSucc != GetTableWithLock(roomid, tableno, table)) {
        ASSERT_FALSE_RETURN;
    }

    const auto result = table->GetChairInfoByUserid(userid, info);
    if (kCommSucc != result) {
        if (kExceptionUserNotOnChair == result) {
            return result;
        }
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int RoomManager::GetRobotCountOnChairs(const RoomID& roomid, const TableNO& tableno, int& count) const {
    std::lock_guard<std::mutex> lock(mutex_);
    TablePtr table;
    if (kCommSucc != GetTableWithLock(roomid, tableno, table)) {
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != table->GetRobotCount(count)) {
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int RoomManager::GetNormalCountOnChairs(const RoomID& roomid, const TableNO& tableno, int& count) const {
    std::lock_guard<std::mutex> lock(mutex_);
    TablePtr table;
    if (kCommSucc != GetTableWithLock(roomid, tableno, table)) {
        ASSERT_FALSE_RETURN;
    }

    const auto all_users = table->GetPlayerCount();

    const auto robot_users = 0;
    if (kCommSucc != table->GetRobotCount(count)) {
        ASSERT_FALSE_RETURN;
    }

    count = all_users - robot_users;
    if (count < 0) ASSERT_FALSE_RETURN;
    return kCommSucc;
}

int RoomManager::IsTablePlaying(const RoomID& roomid, const TableNO& tableno, bool& result) const {
    std::lock_guard<std::mutex> lock(mutex_);
    TablePtr table;
    if (kCommSucc != GetTableWithLock(roomid, tableno, table)) {
        ASSERT_FALSE_RETURN;
    }

    result = (kTablePlaying == table->get_table_status());
    return kCommSucc;
}

int RoomManager::GetMiniPlayers(const RoomID& roomid, int& mini) const {
    std::lock_guard<std::mutex> lock(mutex_);
    RoomPtr room;
    if (kCommSucc != GetRoomWithLock(roomid, room)) {
        ASSERT_FALSE_RETURN;
    }

    mini = room->get_min_playercount_per_table();
    return kCommSucc;
}

int RoomManager::GetRoomWithLock(const RoomID& roomid, RoomPtr& room) const {
    const auto iter = rooms_.find(roomid);
    if (iter == rooms_.end()) {
        ASSERT_FALSE_RETURN;
    }
    room = iter->second;
    return kCommSucc;
}

int RoomManager::GetTableWithLock(const RoomID& roomid, const TableNO& tableno, TablePtr& table) const {
    RoomPtr room;
    if (kCommSucc != GetRoomWithLock(roomid, room)) {
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != room->GetTable(tableno, table)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int RoomManager::SnapShotObjectStatus() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("rooms_ size [%d]", rooms_.size());
    for (auto& kv : rooms_) {
        const auto room = kv.second;
        room->SnapShotObjectStatus();
    }

    return kCommSucc;
}

int RoomManager::GetRoomDepositRange(int64_t& max, int64_t& min) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& kv : rooms_) {
        const auto room = kv.second;
        const auto max_deposit = room->get_max_deposit();
        const auto min_deposit = room->get_min_deposit();
        if (max < max_deposit) max = max_deposit;
        if (min > min_deposit) min = min_deposit;
    }
    if (max < min) { return kCommFaild; }
    if (max <= 0) { return kCommFaild; }
    if (min < 0) { return kCommFaild; }
    return kCommSucc;
}

int RoomManager::GetTableDepositRange(const RoomID& roomid, const TableNO& tableno, int64_t& max, int64_t& min) {
    std::lock_guard<std::mutex> lock(mutex_);
    TablePtr table;
    if (kCommSucc != GetTableWithLock(roomid, tableno, table)) {
        ASSERT_FALSE_RETURN;
    }

    min = table->get_min_deposit();
    max = table->get_max_deposit();

    if (max < min) { return kCommFaild; }
    if (max <= 0) { return kCommFaild; }
    if (min < 0) { return kCommFaild; }

    return kCommSucc;
}
