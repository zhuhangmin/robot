#pragma once
#include "base_room.h"
#include "robot_define.h"

using GameRoomMap = std::hash_map<RoomID, RoomPtr>;

class RoomManager : public ISingletion<RoomManager> {

public:
    int GetRoom(const RoomID& roomid, RoomPtr& room) const;

    int AddRoom(const RoomID& roomid, const RoomPtr &room);

    int Reset();

    int GetChairInfo(const RoomID& roomid, const TableNO& tableno, const int& userid, ChairInfo& info);

    int GetRobotCountOnChairs(const RoomID& roomid, const TableNO& tableno, int& count);

    int GetNormalCountOnChairs(const RoomID& roomid, const TableNO& tableno, int& count);

    const GameRoomMap& GetAllRooms() const;

    int IsTablePlaying(const RoomID& roomid, const TableNO& tableno, bool& result);

    int GetMiniPlayers(const RoomID& roomid, int& mini);

private:
    int GetRoomWithLock(const RoomID& roomid, RoomPtr& room) const;

    int GetTableWithLock(const RoomID& roomid, const TableNO& tableno, TablePtr& table) const;

public:
    // ¶ÔÏó×´Ì¬¿ìÕÕ
    int SnapShotObjectStatus();

protected:
    SINGLETION_CONSTRUCTOR(RoomManager);

private:
    mutable std::mutex mutex_;
    GameRoomMap rooms_;
};

#define RoomMgr RoomManager::Instance()

