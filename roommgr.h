#pragma once
#include "base_room.h"
#include "robot_define.h"
class RoomMgr : public ISingletion<RoomMgr> {
protected:
    SINGLETION_CONSTRUCTOR(RoomMgr);
public:
    virtual std::shared_ptr<BaseRoom> GetRoom(RoomID roomid);
    virtual std::hash_map<RoomID, std::shared_ptr<BaseRoom>> GetAllRooms();
    virtual void AddRoom(RoomID roomid, const std::shared_ptr<BaseRoom> &room);

public:
    void Reset();

private:
    std::mutex rooms_mutex_;
    std::hash_map<RoomID, std::shared_ptr<BaseRoom>> rooms_;
};

