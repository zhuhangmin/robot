#pragma once
#include "base_room.h"
class RoomMgr {
public:
    RoomMgr();
    virtual ~RoomMgr();

    virtual std::shared_ptr<BaseRoom> NewRoom(int roomid = 0);
    virtual std::shared_ptr<BaseRoom> GetRoom(int roomid);
    virtual std::hash_map<int, std::shared_ptr<BaseRoom>> GetAllRooms();
    virtual void AddRoom(int roomid, const std::shared_ptr<BaseRoom> &room);

private:
    std::mutex rooms_mutex_;
    std::hash_map<int, std::shared_ptr<BaseRoom>> rooms_;
};

