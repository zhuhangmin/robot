#include "stdafx.h"
#include "RoomMgr.h"
#include "robot_utils.h"

int RoomMgr::GetRoom(RoomID roomid, std::shared_ptr<BaseRoom>& room) {
    std::lock_guard<std::mutex> lock_g(rooms_mutex_);
    auto itr = rooms_.find(roomid);
    if (itr == rooms_.end()) {
        return kCommFaild;
    }
    room = rooms_[roomid];
    return kCommSucc;
}

int RoomMgr::AddRoom(RoomID roomid, const std::shared_ptr<BaseRoom> &room) {
    CHECK_ROOMID(roomid);
    CHECK_ROOM(room);
    std::lock_guard<std::mutex> lock_g(rooms_mutex_);
    rooms_[roomid] = room;
    return kCommSucc;
}

void RoomMgr::Reset() {
    std::lock_guard<std::mutex> lock_g(rooms_mutex_);
    rooms_.clear();
}

std::hash_map<int, std::shared_ptr<BaseRoom>> RoomMgr::GetAllRooms() {
    std::lock_guard<std::mutex> lock_g(rooms_mutex_);
    return rooms_;
}
