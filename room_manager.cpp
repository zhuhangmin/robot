#include "stdafx.h"
#include "room_manager.h"
#include "robot_utils.h"
#include "setting_manager.h"

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
    if (kCommSucc != AddRoomWithLock(roomid, room)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int RoomManager::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_WARN("Reset RoomManager");
    rooms_.clear();
    return kCommSucc;
}

int RoomManager::ResetDataAndReInit(const game::base::GetGameUsersResp& resp) {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_WARN("RESET Data RoomManger And ReInit");
    rooms_.clear();

    for (auto room_index = 0; room_index < resp.rooms_size(); room_index++) {
        if (kCommSucc != AddRoomPBWithLock(resp.rooms(room_index))) {
            ASSERT_FALSE;
            continue;
        }
    }
    return kCommSucc;
}

int RoomManager::AddRoomPB(const game::base::Room& room_pb) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (kCommSucc != AddRoomPBWithLock(room_pb)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int RoomManager::GetChairInfo(const RoomID& roomid, const TableNO& tableno, const int& userid, ChairInfo& info) const {
    std::lock_guard<std::mutex> lock(mutex_);
    TablePtr table;
    if (kCommSucc != GetTableWithLock(roomid, tableno, table)) {
        ASSERT_FALSE_RETURN;
    }

    const auto result = table->GetChairInfoByUserid(userid, info);
    if (kCommSucc != result) {
        if (kExceptionUserNotOnChair == result) {
            return result;
        }
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int RoomManager::GetRobotCountOnChairs(const RoomID& roomid, const TableNO& tableno, int& count) const {
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

int RoomManager::GetNormalCountOnChairs(const RoomID& roomid, const TableNO& tableno, int& count) const {
    std::lock_guard<std::mutex> lock(mutex_);
    TablePtr table;
    if (kCommSucc != GetTableWithLock(roomid, tableno, table)) {
        ASSERT_FALSE_RETURN;
    }

    const auto all_users = table->GetPlayerCount();

    const auto robot_users = 0;
    if (kCommSucc != table->GetRobotCount(count)) {
        ASSERT_FALSE_RETURN;
    }

    count = all_users - robot_users;
    if (count < 0) ASSERT_FALSE_RETURN;
    return kCommSucc;
}

int RoomManager::IsTablePlaying(const RoomID& roomid, const TableNO& tableno, bool& result) const {
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_ROOMID(roomid);
    CHECK_TABLENO(tableno);
    TablePtr table;
    if (kCommSucc != GetTableWithLock(roomid, tableno, table)) {
        ASSERT_FALSE_RETURN;
    }

    result = (kTablePlaying == table->get_table_status());
    return kCommSucc;
}

int RoomManager::GetMiniPlayers(const RoomID& roomid, int& mini) const {
    std::lock_guard<std::mutex> lock(mutex_);
    RoomPtr room;
    if (kCommSucc != GetRoomWithLock(roomid, room)) {
        ASSERT_FALSE_RETURN;
    }

    mini = room->get_min_playercount_per_table();
    return kCommSucc;
}

int RoomManager::GetRoomWithLock(const RoomID& roomid, RoomPtr& room) const {
    const auto iter = rooms_.find(roomid);
    if (iter == rooms_.end()) {
        ASSERT_FALSE_RETURN;
    }
    room = iter->second;
    return kCommSucc;
}

int RoomManager::GetTableWithLock(const RoomID& roomid, const TableNO& tableno, TablePtr& table) const {
    RoomPtr room;
    if (kCommSucc != GetRoomWithLock(roomid, room)) {
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != room->GetTable(tableno, table)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int RoomManager::AddRoomPBWithLock(const game::base::Room& room_pb) const {
    const auto& room_data_pb = room_pb.room_data();
    const auto roomid = room_data_pb.roomid();

    auto exist = false;
    if (kCommSucc!= SettingMgr.IsRoomSettingExist(roomid, exist)) {
        ASSERT_FALSE_RETURN;
    }
    if (!exist) LOG_WARN("roomid [%d] not eixst in robot.setting", roomid);

    auto room = std::make_shared<BaseRoom>();
    room->set_room_id(room_data_pb.roomid());
    room->set_options(room_data_pb.options());
    room->set_configs(room_data_pb.configs());
    room->set_manages(room_data_pb.manages());
    room->set_max_table_cout(room_data_pb.max_table_cout());
    room->set_chaircount_per_table(room_data_pb.chaircount_per_table());
    room->set_min_playercount_per_table(room_data_pb.min_player_count());
    room->set_min_deposit(room_data_pb.min_deposit());
    room->set_max_deposit(room_data_pb.max_deposit());

    // TABLE
    const auto size = room_pb.tables_size();
    for (auto table_index = 0; table_index < size; table_index++) {
        const auto& table_pb = room_pb.tables(table_index);
        const auto tableno = table_pb.tableno();
        auto table = std::make_shared<Table>();
        AddTablePBWithLock(table_pb, table);
        room->AddTable(tableno, table);
    }

    // ROOM
    // TODO REMOVE RoomMgr.
    if (kCommSucc != RoomMgr.AddRoomWithLock(roomid, room)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int RoomManager::AddTablePBWithLock(const game::base::Table& table_pb, const TablePtr& table) const {
    table->set_table_no(table_pb.tableno()); // tableno start from 1
    table->set_room_id(table_pb.roomid());
    table->set_chair_count(table_pb.chair_count());
    table->set_banker_chair(table_pb.banker_chair());
    table->set_min_deposit(table_pb.min_deposit());
    table->set_max_deposit(table_pb.max_deposit());
    table->set_base_deposit(table_pb.base_deposit());
    table->set_table_status(table_pb.table_status());

    // CHAIR
    for (auto chair_index = 0; chair_index < table_pb.chairs_size(); chair_index++) {
        const auto& chair_pb = table_pb.chairs(chair_index);
        const auto chairno = chair_pb.chairno(); // chairno start from 1
        const auto userid = chair_pb.userid();
        if (userid < 0) ASSERT_FALSE_RETURN;
        if (0 == userid) continue;
        ChairInfo chair_info;
        chair_info.set_userid(userid);
        chair_info.set_chair_no(chairno);
        chair_info.set_chair_status(static_cast<ChairStatus>(chair_pb.chair_status()));
        chair_info.set_bind_timestamp(chair_pb.bind_timestamp());
        table->AddChair(chairno, chair_info);
    }

    // TABLE USER INFO
    for (auto table_user_index = 0; table_user_index < table_pb.table_users_size(); table_user_index++) {
        const auto& table_user_pb = table_pb.table_users(table_user_index);
        const auto userid = table_user_pb.userid();
        if (userid < 0) ASSERT_FALSE_RETURN;
        if (0 == userid) continue;
        TableUserInfo table_user_info;
        table_user_info.set_userid(table_user_pb.userid());
        table_user_info.set_user_type(table_user_pb.user_type());
        table_user_info.set_bind_timestamp(table_user_pb.bind_timestamp());
        table->AddTableUserInfo(userid, table_user_info);
    }

    return kCommSucc;
}

int RoomManager::AddRoomWithLock(const RoomID& roomid, const RoomPtr &room) {
    CHECK_ROOMID(roomid);
    CHECK_ROOM(room);
    rooms_[roomid] = room;
    return kCommSucc;
}

int RoomManager::SnapShotObjectStatus() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("rooms_ size [%d]", rooms_.size());
    for (auto& kv : rooms_) {
        const auto room = kv.second;
        room->SnapShotObjectStatus();
    }

    return kCommSucc;
}

int RoomManager::GetRoomDepositRange(int64_t& max, int64_t& min) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& kv : rooms_) {
        const auto room = kv.second;
        const auto max_deposit = room->get_max_deposit();
        const auto min_deposit = room->get_min_deposit();
        if (max < max_deposit) max = max_deposit;
        if (min > min_deposit) min = min_deposit;
    }
    if (max < min) { return kCommFaild; }
    if (max <= 0) { return kCommFaild; }
    if (min < 0) { return kCommFaild; }
    return kCommSucc;
}

int RoomManager::GetTableDepositRange(const RoomID& roomid, const TableNO& tableno, int64_t& max, int64_t& min) {
    std::lock_guard<std::mutex> lock(mutex_);
    TablePtr table;
    if (kCommSucc != GetTableWithLock(roomid, tableno, table)) {
        ASSERT_FALSE_RETURN;
    }

    min = table->get_min_deposit();
    max = table->get_max_deposit();

    if (max < min) { return kCommFaild; }
    if (max <= 0) { return kCommFaild; }
    if (min < 0) { return kCommFaild; }

    return kCommSucc;
}
