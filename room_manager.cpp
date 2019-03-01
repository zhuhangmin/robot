#include "stdafx.h"
#include "room_manager.h"
#include "robot_utils.h"

int RoomManager::GetRoom(const RoomID roomid, RoomPtr& room) const {
    std::lock_guard<std::mutex> lock(rooms_mutex_);
    auto iter = rooms_.find(roomid);
    if (iter == rooms_.end()) {
        return kCommFaild;
    }
    room = iter->second;
    return kCommSucc;
}

int RoomManager::AddRoom(const RoomID roomid, const RoomPtr &room) {
    CHECK_ROOMID(roomid);
    CHECK_ROOM(room);
    std::lock_guard<std::mutex> lock(rooms_mutex_);
    rooms_[roomid] = room;
    return kCommSucc;
}

int RoomManager::Reset() {
    std::lock_guard<std::mutex> lock(rooms_mutex_);
    rooms_.clear();
    return kCommSucc;
}

const GameRoomMap& RoomManager::GetAllRooms() const {
    std::lock_guard<std::mutex> lock(rooms_mutex_);
    return rooms_;
}

int RoomManager::SnapShotObjectStatus() {
    std::lock_guard<std::mutex> lock(rooms_mutex_);
    LOG_INFO("OBJECT ADDRESS [%x]", this);
    LOG_INFO("rooms_ size [%d]", rooms_.size());
    for (auto& kv : rooms_) {
        auto roomid = kv.first;
        auto room = kv.second;
        LOG_INFO("roomid [%d]", roomid);
        room->SnapShotObjectStatus();
    }

    return kCommSucc;
}
