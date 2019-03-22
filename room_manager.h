#pragma once
#include "base_room.h"
#include "robot_define.h"

using GameRoomMap = std::hash_map<RoomID, RoomPtr>;

class RoomManager : public ISingletion<RoomManager> {
public:
    int GetRoom(const RoomID& roomid, RoomPtr& room) const;

    int AddRoom(const RoomID& roomid, const RoomPtr &room);

    int Reset();

    int ResetDataAndReInit(const game::base::GetGameUsersResp& resp);

    int GetChairInfo(const RoomID& roomid, const TableNO& tableno, const int& userid, ChairInfo& info) const;

    int GetRobotCountOnChairs(const RoomID& roomid, const TableNO& tableno, int& count) const;

    int GetNormalCountOnChairs(const RoomID& roomid, const TableNO& tableno, int& count) const;

    int IsTablePlaying(const RoomID& roomid, const TableNO& tableno, bool& result) const;

    // 最小开桌人数
    int GetMiniPlayers(const RoomID& roomid, int& mini) const;

    // 所有房银区间 //TODO 提供特定房间的接口
    int GetRoomDepositRange(int64_t& max, int64_t& min) const;

    // 桌银区间
    int GetTableDepositRange(const RoomID& roomid, const TableNO& tableno, int64_t& max, int64_t& min);

    int AddRoomPB(const game::base::Room& room_pb) const;

private:
    int GetRoomWithLock(const RoomID& roomid, RoomPtr& room) const;

    int GetTableWithLock(const RoomID& roomid, const TableNO& tableno, TablePtr& table) const;

    int AddRoomPBWithLock(const game::base::Room& room_pb) const;

    int AddTablePBWithLock(const game::base::Table& table_pb, const TablePtr& table) const;

    int AddRoomWithLock(const RoomID& roomid, const RoomPtr& room);


public:
    // 对象状态快照
    int SnapShotObjectStatus();

protected:
    SINGLETION_CONSTRUCTOR(RoomManager);

private:
    // 数据锁
    mutable std::mutex mutex_;

    // 房间数据
    GameRoomMap rooms_;
};

#define RoomMgr RoomManager::Instance()

