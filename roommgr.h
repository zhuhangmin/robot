#pragma once
#include "base_room.h"
#include "RobotDef.h"
class RoomMgr : public ISingletion<RoomMgr> {
protected:
    SINGLETION_CONSTRUCTOR(RoomMgr);
public:
    virtual std::shared_ptr<BaseRoom> NewRoom(int roomid = 0);
    virtual std::shared_ptr<BaseRoom> GetRoom(int roomid);
    virtual std::hash_map<int, std::shared_ptr<BaseRoom>> GetAllRooms();
    virtual void AddRoom(int roomid, const std::shared_ptr<BaseRoom> &room);

public:
    void Reset();

private:
    std::mutex rooms_mutex_;
    std::hash_map<int, std::shared_ptr<BaseRoom>> rooms_;
};

