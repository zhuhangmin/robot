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

int RoomManager::GetChairInfo(const RoomID& roomid, const TableNO& tableno, const int& userid, ChairInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_);
    TablePtr table;
    if (kCommSucc != GetTableWithLock(roomid, tableno, table)) {
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != table->GetChairInfoByUserid(userid, info)) {
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int RoomManager::GetRobotCountOnChairs(const RoomID& roomid, const TableNO& tableno, int& count) {
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

const GameRoomMap& RoomManager::GetAllRooms() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return rooms_;
}

int RoomManager::GetRoomWithLock(const RoomID& roomid, RoomPtr& room) const {
    const auto iter = rooms_.find(roomid);
    if (iter == rooms_.end()) {
        ASSERT_FALSE_RETURN;
    }
    room = iter->second;
    return kCommSucc;
}

int RoomManager::GetTableWithLock(const RoomID roomid, const TableNO& tableno, TablePtr& table) const {
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
        const auto roomid = kv.first;
        const auto room = kv.second;
        room->SnapShotObjectStatus();
    }

    return kCommSucc;
}
