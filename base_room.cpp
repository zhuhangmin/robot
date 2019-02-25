#include "stdafx.h"
#include "base_room.h"
#include "usermgr.h"

BaseRoom::BaseRoom() {
    //TODO HOW TO ADD TABLE , WILL ALL THE TABLE BE NOTIFY ON INIT GAMEINFO ?
    //CreateTables();
}


BaseRoom::BaseRoom(int roomid) {
    set_room_id(roomid);
    //CreateTables();
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

int BaseRoom::PlayerEnterGame(const std::shared_ptr<User> &user) {
    auto tableno = user->get_table_no();
    std::shared_ptr<Table> table = GetTable(tableno);
    int ret = table->BindPlayer(user);
    if (ret != kCommSucc) {
        UWL_WRN("BindPlayer faild. userid=%d, tableno=%d", user->get_user_id(), tableno);
        return kCommFaild;
    }

    return kCommSucc;
}

int BaseRoom::BindPlayer(const std::shared_ptr<User> &user) {
    //V524 It is odd that the body of 'BindPlayer' function is fully equivalent to the body of 'PlayerEnterGame' function.base_room.cpp 43
    auto tableno = user->get_table_no();
    std::shared_ptr<Table> table = GetTable(tableno);
    int ret = table->BindPlayer(user);
    if (ret != kCommSucc) {
        UWL_WRN("BindPlayer faild. userid=%d, tableno=%d", user->get_user_id(), tableno);
        return kCommFaild;
    }
    return kCommSucc;
}

int BaseRoom::ContinueGame(const std::shared_ptr<User> &user) {
    auto table = GetTable(user->get_table_no());
    if (!IsValidTable(table->get_table_no())) {
        UWL_WRN("Get table[%d] faild", user->get_table_no());
        return kCommFaild;
    }

    // �������ң�У���������Ƿ��Ѿ���������
    if (user->get_chair_no() > 0) {
        int seat_userid = table->GetUserID(user->get_chair_no());
        if (seat_userid != user->get_user_id()) {
            UWL_WRN("user[%d] has in table[%d], chair[%d]. user[%d] continue game faild.",
                    seat_userid, user->get_table_no(), user->get_chair_no(), user->get_user_id());
            return kCommFaild;
        }
    }

    // ���������Ϣ
    UserMgr::Instance().AddUser(user->get_user_id(), user);
    return kCommSucc;
}

int BaseRoom::UserLeaveGame(int userid, int tableno) {
    auto table = GetTable(tableno);
    if (!IsValidTable(table->get_table_no())) {
        UWL_WRN("Get table[%d] faild", tableno);
        return kCommFaild;
    }

    {
        table->UnbindUser(userid);
    }
    return kCommSucc;
}

int BaseRoom::UnbindUser(int userid, int tableno) {
    //V524 It is odd that the body of 'UnbindUser' function is fully equivalent to the body of 'UserLeaveGame' function.base_room.cpp 91
    auto table = GetTable(tableno);
    if (!IsValidTable(table->get_table_no())) {
        UWL_WRN("Get table[%d] faild", tableno);
        return kCommFaild;
    }

    {
        table->UnbindUser(userid);
    }
    return kCommSucc;
}

int BaseRoom::UserGiveUp(int userid, int tableno) {
    auto table = GetTable(tableno);
    if (!IsValidTable(table->get_table_no())) {
        UWL_WRN("Get table[%d] faild", tableno);
        return kCommFaild;
    }

    return kCommSucc;
}

int BaseRoom::Looker2Player(std::shared_ptr<User> &user) {
    if (!IsValidDeposit(user->get_deposit())) {
        UWL_WRN("user[%d] deposit[%I64d] invalid.", user->get_user_id(), user->get_deposit());
        return kInvalidDeposit;
    }

    auto table = GetTable(user->get_table_no());
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

int BaseRoom::Player2Looker(std::shared_ptr<User> &user) {
    auto table = GetTable(user->get_table_no());
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


    table->UnbindPlayer(user->get_user_id());

    return kCommSucc;
}

int BaseRoom::LookerEnterGame(const std::shared_ptr<User> &user) {
    TableNO target_tableno = user->get_table_no();
    if (false == IsValidTable(target_tableno)) {
        UWL_WRN("Get table[%d] faild", target_tableno);
        return kCommFaild;
    }

    auto tableno = user->get_table_no();
    std::shared_ptr<Table> table = GetTable(tableno);
    table->BindLooker(user);

    return kCommSucc;
}

bool BaseRoom::IsValidTable(int tableno) {
    if (tableno <= 0 || tableno > get_max_table_cout()) {
        return false;
    }
    return true;
}

bool BaseRoom::IsValidDeposit(INT64 deposit) {
    if (deposit >= get_min_deposit() && deposit < get_max_deposit()) {
        return true;
    }
    return false;
}

void BaseRoom::AddTable(TableNO tableno, std::shared_ptr<Table> table) {
    assert(tableno >= 1);
    tables_[tableno - 1] = table;
}

std::shared_ptr<Table> BaseRoom::GetTable(int tableno) {
    static std::shared_ptr<Table> null_table = std::make_shared<Table>();

    if (tableno <= 0 || tableno > max_table_cout_) {
        UWL_WRN("GetTable faild. valid tableno[%d]", tableno);
        return null_table;
    }

    // ���Ŵ�1��ʼ���±��0��ʼ
    return tables_.at(tableno - 1);
}

int BaseRoom::GetEligibleTable(const std::shared_ptr<User> &user, int &min_tableno, int &max_tableno) {
    // ��Ϸ���ɸ����Լ��������ش˺���������˵�������Ƶȣ�

    min_tableno = 1;	// ���Ŵ�1��ʼ
    max_tableno = max_table_cout_;

    if (min_tableno <= 0 || max_tableno <= 0 || max_tableno - min_tableno <= 0) {
        return kCommFaild;
    }

    return kCommSucc;
}
