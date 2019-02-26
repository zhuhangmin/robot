#include "stdafx.h"
#include "base_room.h"
#include "usermgr.h"
#include "robot_utils.h"

BaseRoom::BaseRoom() {}

BaseRoom::BaseRoom(int roomid) {
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
    } else if (IS_BIT_SET(options_, kPrivateRoom)) {
        return kPrivateRoom;
    } else if (IS_BIT_SET(options_, kMatchRoom)) {
        return kMatchRoom;
    } else {
        return kUnkonwRoom;
    }
}

int BaseRoom::PlayerEnterGame(const UserPtr &user) {
    CHECK_USER(user);
    auto tableno = user->get_table_no();

    TablePtr table;
    if (kCommSucc != GetTable(tableno, table)) {
        UWL_WRN("GetTable faild. userid=%d, tableno=%d", user->get_user_id(), tableno);
        return kCommFaild;
    }

    if (kCommSucc != table->BindPlayer(user)) {
        UWL_WRN("BindPlayer faild. userid=%d, tableno=%d", user->get_user_id(), tableno);
        return kCommFaild;
    }

    return kCommSucc;
}

int BaseRoom::BindPlayer(const UserPtr &user) {
    CHECK_USER(user);
    //V524 It is odd that the body of 'BindPlayer' function is fully equivalent to the body of 'PlayerEnterGame' function.base_room.cpp 43
    auto tableno = user->get_table_no();
    TablePtr table;
    if (kCommSucc != GetTable(tableno, table)) {
        UWL_WRN("GetTable faild. userid=%d, tableno=%d", user->get_user_id(), tableno);
        return kCommFaild;
    }

    if (kCommSucc != table->BindPlayer(user)) {
        UWL_WRN("BindPlayer faild. userid=%d, tableno=%d", user->get_user_id(), tableno);
        return kCommFaild;
    }
    return kCommSucc;
}

int BaseRoom::UserLeaveGame(const UserID userid, const TableNO tableno) {
    CHECK_USERID(userid);
    CHECK_TABLENO(tableno);

    TablePtr table;
    if (kCommSucc != GetTable(tableno, table)) {
        UWL_WRN("GetTable faild. userid=%d, tableno=%d", userid, tableno);
        return kCommFaild;
    }

    if (!IsValidTable(table->get_table_no())) {
        UWL_WRN("Get table[%d] faild", tableno);
        return kCommFaild;
    }

    if (kCommSucc != table->UnbindUser(userid)) {
        assert(false);
        return kCommFaild;
    }
    return kCommSucc;
}

int BaseRoom::UnbindUser(const UserID userid, const TableNO tableno) {
    CHECK_USERID(userid);
    CHECK_TABLENO(tableno);

    //V524 It is odd that the body of 'UnbindUser' function is fully equivalent to the body of 'UserLeaveGame' function.base_room.cpp 91
    TablePtr table;
    if (kCommSucc != GetTable(tableno, table)) {
        UWL_WRN("GetTable faild. userid=%d, tableno=%d", userid, tableno);
        return kCommFaild;
    }

    if (!IsValidTable(table->get_table_no())) {
        UWL_WRN("Get table[%d] faild", tableno);
        return kCommFaild;
    }

    if (kCommSucc != table->UnbindUser(userid)) {
        assert(false);
        return kCommFaild;
    }
    return kCommSucc;
}

int BaseRoom::UserGiveUp(const UserID userid, const TableNO tableno) {
    CHECK_USERID(userid);
    CHECK_TABLENO(tableno);

    TablePtr table;
    if (kCommSucc != GetTable(tableno, table)) {
        UWL_WRN("GetTable faild. userid=%d, tableno=%d", userid, tableno);
        return kCommFaild;
    }

    if (!IsValidTable(table->get_table_no())) {
        UWL_WRN("Get table[%d] faild", tableno);
        return kCommFaild;
    }

    return kCommSucc;
}

int BaseRoom::Looker2Player(UserPtr &user) {
    CHECK_USER(user);
    if (!IsValidDeposit(user->get_deposit())) {
        UWL_WRN("user[%d] deposit[%I64d] invalid.", user->get_user_id(), user->get_deposit());
        return kInvalidDeposit;
    }

    auto tableno = user->get_table_no();
    TablePtr table;
    if (kCommSucc != GetTable(tableno, table)) {
        UWL_WRN("GetTable faild. userid=%d, tableno=%d", user->get_user_id(), tableno);
        return kCommFaild;
    }

    if (!IsValidTable(table->get_table_no())) {
        UWL_WRN("Get table[%d] faild", user->get_table_no());
        return kCommFaild;
    }

    if (!table->IsTableUser(user->get_user_id())) {
        UWL_WRN("user[%d] is not in table[%d]", user->get_user_id(), user->get_table_no());
        return kCommFaild;
    }

    if (table->IsTablePlayer(user->get_user_id())) {
        UWL_WRN("Looker2Player succ. but user[%d] is player.", user->get_user_id());
        user->set_chair_no(table->GetUserChair(user->get_user_id()));
        return kCommSucc;
    }

    int ret = table->BindPlayer(user);
    if (ret != kCommSucc) {
        UWL_WRN("user[%d] BindPlayer faild. ret=%d", user->get_user_id(), ret);
        return kCommFaild;
    }

    return kCommSucc;
}

int BaseRoom::Player2Looker(UserPtr &user) {
    CHECK_USER(user);
    auto tableno = user->get_table_no();
    TablePtr table;
    if (kCommSucc != GetTable(tableno, table)) {
        UWL_WRN("GetTable faild. userid=%d, tableno=%d", user->get_user_id(), tableno);
        return kCommFaild;
    }

    if (!IsValidTable(table->get_table_no())) {
        UWL_WRN("Get table[%d] faild", user->get_table_no());
        return kCommFaild;
    }

    if (!table->IsTablePlayer(user->get_user_id())) {
        UWL_WRN("user[%d] is not in table[%d]", user->get_user_id(), user->get_table_no());
        return kCommFaild;
    }

    if (IS_BIT_SET(table->get_table_status(), kTablePlaying)) {
        (void) UserGiveUp(user->get_user_id(), user->get_table_no());
    }


    if (kCommSucc != table->UnbindPlayer(user->get_user_id())) {
        assert(false);
        return kCommFaild;
    }

    return kCommSucc;
}

int BaseRoom::LookerEnterGame(const UserPtr &user) {
    CHECK_USER(user);
    TableNO target_tableno = user->get_table_no();
    if (false == IsValidTable(target_tableno)) {
        UWL_WRN("Get table[%d] faild", target_tableno);
        return kCommFaild;
    }

    auto tableno = user->get_table_no();
    TablePtr table;
    if (kCommSucc != GetTable(tableno, table)) {
        UWL_WRN("GetTable faild. userid=%d, tableno=%d", user->get_user_id(), tableno);
        return kCommFaild;
    }
    if (kCommSucc != table->BindLooker(user)) {
        assert(false);
        return kCommFaild;
    }

    return kCommSucc;
}

bool BaseRoom::IsValidTable(TableNO tableno) const {
    if (tableno <= 0 || tableno > get_max_table_cout()) {
        return false;
    }
    return true;
}

bool BaseRoom::IsValidDeposit(INT64 deposit) const {
    if (deposit >= get_min_deposit() && deposit < get_max_deposit()) {
        return true;
    }
    return false;
}

int BaseRoom::AddTable(const TableNO tableno, const TablePtr& table) {
    CHECK_TABLENO(tableno);
    CHECK_TABLE(table);
    tables_[tableno - 1] = table;
    return kCommSucc;
}

int BaseRoom::GetTable(const TableNO tableno, TablePtr& table) const {
    CHECK_TABLENO(tableno);
    if (!IsValidTable(tableno)) {
        UWL_WRN("GetTable faild. valid tableno[%d]", tableno);
        return kCommFaild;
    }

    // 桌号从1开始，下标从0开始
    table = tables_.at(tableno - 1);
    return kCommSucc;
}
