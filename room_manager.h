#pragma once
#include "base_room.h"
#include "robot_define.h"

using GameRoomMap = std::hash_map<RoomID, RoomPtr>;

class RoomManager : public ISingletion<RoomManager> {
protected:
    SINGLETION_CONSTRUCTOR(RoomManager);
public:
    int GetRoom(const RoomID& roomid, RoomPtr& room) const;
    const GameRoomMap& GetAllRooms() const;
    int AddRoom(const RoomID roomid, const RoomPtr &room);

public:
    int Reset();

public:
    // ¶ÔÏó×´Ì¬¿ìÕÕ
    int SnapShotObjectStatus();

private:
    mutable std::mutex rooms_mutex_;
    GameRoomMap rooms_;
};

#define RoomMgr RoomManager::Instance()

