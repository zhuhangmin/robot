#pragma once
#include "base_room.h"
#include "robot_define.h"

using GameRoomMap = std::hash_map<RoomID, RoomPtr>;

class RoomManager : public ISingletion<RoomManager> {
public:
    int GetRoom(const RoomID& roomid, RoomPtr& room) const;

    int AddRoom(const RoomID& roomid, const RoomPtr &room);

    int Reset();

    int GetChairInfo(const RoomID& roomid, const TableNO& tableno, const int& userid, ChairInfo& info) const;

    int GetRobotCountOnChairs(const RoomID& roomid, const TableNO& tableno, int& count) const;

    int GetNormalCountOnChairs(const RoomID& roomid, const TableNO& tableno, int& count) const;

    int IsTablePlaying(const RoomID& roomid, const TableNO& tableno, bool& result) const;

    int GetMiniPlayers(const RoomID& roomid, int& mini) const;

    int GetRoomDepositRange(int64_t& max, int64_t& min) const;

    int GetTableDepositRange(const RoomID& roomid, const TableNO& tableno, int64_t& max, int64_t& min);

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

