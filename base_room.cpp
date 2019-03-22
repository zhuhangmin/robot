#include "stdafx.h"
#include "base_room.h"
#include "robot_utils.h"

BaseRoom::BaseRoom() {}

BaseRoom::BaseRoom(const int& roomid) {
    if (RobotUtils::IsValidRoomID(roomid)) {
        assert(roomid);
        return;
    }
    set_room_id(roomid);
}

BaseRoom::~BaseRoom() {}

RoomOptional BaseRoom::GetRoomType() {
    if (IS_BIT_SET(options_, kClassicsRoom)) {
        return kClassicsRoom;
    }

    if (IS_BIT_SET(options_, kPrivateRoom)) {
        return kPrivateRoom;
    }

    if (IS_BIT_SET(options_, kMatchRoom)) {
        return kMatchRoom;
    }

    return kUnkonwRoom;
}

int BaseRoom::PlayerEnterGame(const UserPtr &user) {
    CHECK_USER(user);
    const auto tableno = user->get_table_no();

    TablePtr table;
    if (kCommSucc != GetTable(tableno, table)) {
        LOG_WARN("GetTable faild. userid  [%d], tableno  [%d]", user->get_user_id(), tableno);
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != table->BindPlayer(user)) {
        LOG_WARN("BindPlayer faild. userid  [%d], tableno  [%d]", user->get_user_id(), tableno);
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int BaseRoom::BindPlayer(const UserPtr& user) const {
    CHECK_USER(user);
    //V524 It is odd that the body of 'BindPlayer' function is fully equivalent to the body of 'PlayerEnterGame' function.base_room.cpp 43
    const auto tableno = user->get_table_no();
    TablePtr table;
    if (kCommSucc != GetTable(tableno, table)) {
        LOG_WARN("GetTable faild. userid  [%d], tableno  [%d]", user->get_user_id(), tableno);
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != table->BindPlayer(user)) {
        LOG_WARN("BindPlayer faild. userid  [%d], tableno  [%d]", user->get_user_id(), tableno);
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int BaseRoom::UserLeaveGame(const UserID& userid, const TableNO& tableno) {
    CHECK_USERID(userid);
    CHECK_TABLENO(tableno);

    TablePtr table;
    if (kCommSucc != GetTable(tableno, table)) {
        LOG_WARN("GetTable faild. userid  [%d], tableno  [%d]", userid, tableno);
        ASSERT_FALSE_RETURN;
    }

    if (!IsValidTable(table->get_table_no())) {
        LOG_WARN("Get table[%d] faild", tableno);
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != table->UnbindUser(userid)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int BaseRoom::UnbindUser(const UserID& userid, const TableNO& tableno) const {
    CHECK_USERID(userid);
    CHECK_TABLENO(tableno);

    //V524 It is odd that the body of 'UnbindUser' function is fully equivalent to the body of 'UserLeaveGame' function.base_room.cpp 91
    TablePtr table;
    if (kCommSucc != GetTable(tableno, table)) {
        LOG_WARN("GetTable faild. userid  [%d], tableno  [%d]", userid, tableno);
        ASSERT_FALSE_RETURN;
    }

    if (!IsValidTable(table->get_table_no())) {
        LOG_WARN("Get table[%d] faild", tableno);
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != table->UnbindUser(userid)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int BaseRoom::UserGiveUp(const UserID& userid, const TableNO& tableno) {
    CHECK_USERID(userid);
    CHECK_TABLENO(tableno);

    TablePtr table;
    if (kCommSucc != GetTable(tableno, table)) {
        LOG_WARN("GetTable faild. userid  [%d], tableno  [%d]", userid, tableno);
        ASSERT_FALSE_RETURN;
    }

    if (!IsValidTable(table->get_table_no())) {
        LOG_WARN("Get table[%d] faild", tableno);
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int BaseRoom::Looker2Player(const UserPtr& user) {
    CHECK_USER(user);

    const auto tableno = user->get_table_no();
    TablePtr table;
    if (kCommSucc != GetTable(tableno, table)) {
        LOG_WARN("GetTable faild. userid  [%d], tableno  [%d]", user->get_user_id(), tableno);
        ASSERT_FALSE_RETURN;
    }

    if (!IsValidTable(table->get_table_no())) {
        LOG_WARN("Get table[%d] faild", user->get_table_no());
        ASSERT_FALSE_RETURN;
    }

    if (!table->IsTableUser(user->get_user_id())) {
        LOG_WARN("user[%d] is not in table[%d]", user->get_user_id(), user->get_table_no());
        ASSERT_FALSE_RETURN;
    }

    if (table->IsTablePlayer(user->get_user_id())) {
        LOG_WARN("Looker2Player succ. but user[%d] is player.", user->get_user_id());
        user->set_chair_no(table->GetUserChair(user->get_user_id()));
        return kCommSucc;
    }

    const auto ret = table->BindPlayer(user);
    if (kCommSucc != ret) {
        LOG_WARN("user[%d] BindPlayer faild. ret  [%d]", user->get_user_id(), ret);
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int BaseRoom::Player2Looker(const UserPtr& user) {
    CHECK_USER(user);
    const auto tableno = user->get_table_no();
    TablePtr table;
    if (kCommSucc != GetTable(tableno, table)) {
        LOG_WARN("GetTable faild. userid  [%d], tableno  [%d]", user->get_user_id(), tableno);
        ASSERT_FALSE_RETURN;
    }

    if (!IsValidTable(table->get_table_no())) {
        LOG_WARN("Get table[%d] faild", user->get_table_no());
        ASSERT_FALSE_RETURN;
    }

    if (!table->IsTablePlayer(user->get_user_id())) {
        LOG_WARN("user[%d] is not in table[%d]", user->get_user_id(), user->get_table_no());
        ASSERT_FALSE_RETURN;
    }

    if (IS_BIT_SET(table->get_table_status(), kTablePlaying)) {
        (void) UserGiveUp(user->get_user_id(), user->get_table_no());
    }


    if (kCommSucc != table->UnbindPlayer(user->get_user_id())) {
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int BaseRoom::SwitchTable(const UserPtr& user, const TableNO& tableno) {
    const auto userid = user->get_user_id();
    if (kCommSucc != UnbindUser(userid, tableno)) {
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != BindPlayer(user)) {
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int BaseRoom::StartGame(const TableNO& tableno) {
    TablePtr table;
    if (kCommSucc != GetTable(tableno, table)) {
        LOG_WARN("GetTable faild. tableno  [%d]", tableno);
        ASSERT_FALSE_RETURN;
    }

    if (!IsValidTable(table->get_table_no())) {
        LOG_WARN("Get table [%d] faild", tableno);
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != table->StartGame()) {
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int BaseRoom::LookerEnterGame(const UserPtr &user) {
    CHECK_USER(user);
    const auto target_tableno = user->get_table_no();
    if (!IsValidTable(target_tableno)) {
        LOG_WARN("Get table[%d] faild", target_tableno);
        ASSERT_FALSE_RETURN;
    }

    const auto tableno = user->get_table_no();
    TablePtr table;
    if (kCommSucc != GetTable(tableno, table)) {
        LOG_WARN("GetTable faild. userid  [%d], tableno  [%d]", user->get_user_id(), tableno);
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != table->BindLooker(user)) {
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

bool BaseRoom::IsValidTable(const TableNO& tableno) const {
    if (tableno <= 0 || tableno > get_max_table_cout()) {
        return false;
    }
    return true;
}

bool BaseRoom::IsValidDeposit(const INT64& deposit) const {
    if (deposit >= get_min_deposit() && deposit < get_max_deposit()) {
        return true;
    }
    return false;
}

int BaseRoom::AddTable(const TableNO& tableno, const TablePtr& table) {
    CHECK_TABLENO(tableno);
    CHECK_TABLE(table);
    tables_[tableno - 1] = table;
    return kCommSucc;
}

int BaseRoom::GetTable(const TableNO& tableno, TablePtr& table) const {
    CHECK_TABLENO(tableno);
    if (!IsValidTable(tableno)) {
        LOG_WARN("GetTable faild. valid tableno[%d]", tableno);
        ASSERT_FALSE_RETURN;
    }

    // 桌号从1开始，下标从0开始
    table = tables_.at(tableno - 1);
    return kCommSucc;
}

int BaseRoom::SnapShotObjectStatus() {
#ifdef _DEBUG
    LOG_INFO("roomid [%d] options [%d] configs [%d] min_playercount_per_table [%d] chaircount_per_table [%d]  manages [%d] max_table_cout [%d] min_deposit [%I64d] max_deposit [%I64d]",
             room_id_, options_, configs_, min_playercount_per_table_, chaircount_per_table_, manages_, max_table_cout_, min_deposit_, max_deposit_);
    LOG_INFO("tables_ size [%d]", max_table_cout_);
    /*for (auto i = 0; i < max_table_cout_; i = i + 200) {
        if (tables_[i])
        tables_[i]->SnapShotObjectStatus();
        }*/
    /*if (tables_[0])
        tables_[0]->SnapShotObjectStatus();
        if (tables_[1])
        tables_[1]->SnapShotObjectStatus();*/
#endif
    return kCommSucc;
}
