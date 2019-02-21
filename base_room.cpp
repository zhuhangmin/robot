#include "stdafx.h"
#include "base_room.h"
#include "usermgr.h"

BaseRoom::BaseRoom() {
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
    std::lock_guard<std::mutex> lock_g(alloc_table_chair_mutex_);

    auto tableno = user->get_table_no();
    std::shared_ptr<Table> table = GetTable(tableno);
    int ret = table->BindPlayer(user);
    if (ret != kCommSucc) {
        UWL_WRN("BindPlayer faild. userid=%d, tableno=%d", user->get_user_id(), tableno);
        return kCommFaild;
    }

    UserMgr::Instance().AddUser(user->get_user_id(), user);

    return kCommSucc;
}

int BaseRoom::BindPlayer(const std::shared_ptr<User> &user) {
    std::lock_guard<std::mutex> lock_g(alloc_table_chair_mutex_);

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

    // 如果是玩家，校验椅子上是否已经有其他人
    if (user->get_chair_no() > 0) {
        int seat_userid = table->GetUserID(user->get_chair_no());
        if (seat_userid != user->get_user_id()) {
            UWL_WRN("user[%d] has in table[%d], chair[%d]. user[%d] continue game faild.",
                    seat_userid, user->get_table_no(), user->get_chair_no(), user->get_user_id());
            return kCommFaild;
        }
    }

    // 更新玩家信息
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

    std::lock_guard<std::mutex> lock_g(alloc_table_chair_mutex_);

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

int BaseRoom::SwitchTable(std::shared_ptr<User> &user) {
    // 解除和原桌子的关系
    {
        auto table = GetTable(user->get_table_no());
        if (!IsValidTable(table->get_table_no())) {
            UWL_WRN("Get table[%d] faild", user->get_table_no());
            return kCommFaild;
        }

        if (IS_BIT_SET(table->get_table_status(), kTablePlaying) && table->IsTablePlayer(user->get_user_id())) {
            (void) UserGiveUp(user->get_user_id(), user->get_table_no());
        }
        table->UnbindUser(user->get_user_id());

        // 填充新桌椅信息  TODO:要更新usermanager里面的user
        user->set_table_no(0);
        user->set_chair_no(0);
    }

    // TODO
    // 绑定新桌子
    {
        //std::lock_guard<std::mutex> lock_g(alloc_table_chair_mutex_);

        //// 获取一个有效桌子
        //int tableno = GetAvailableTableno(user, user->get_table_no());

        //// 玩家上桌
        //auto table = GetTable(tableno);
        //if (!IsValidTable(table->get_table_no())) {
        //    UWL_WRN("Get table[%d] faild", tableno);
        //    return kCommFaild;
        //}

        //int chairno = 0;
        //int ret = table->BindPlayer(user);
        //if (ret != kCommSucc) {
        //    UWL_WRN("BindPlayer faild. user[%d], table[%d]", user->get_user_id(), tableno);
        //    return kCommFaild;
        //}

        //// 填充新桌椅信息  TODO:要更新usermanager里面的user
        //user->set_table_no(tableno);
        //user->set_chair_no(chairno);
        //user->set_enter_timestamp(time(0));
    }

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

    UserMgr::Instance().AddUser(user->get_user_id(), user);
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

    // 桌号从1开始，下标从0开始
    return tables_.at(tableno - 1);
}

int BaseRoom::GetEligibleTable(const std::shared_ptr<User> &user, int &min_tableno, int &max_tableno) {
    // 游戏方可根据自己需求重载此函数（比如说桌银限制等）

    min_tableno = 1;	// 桌号从1开始
    max_tableno = max_table_cout_;

    if (min_tableno <= 0 || max_tableno <= 0 || max_tableno - min_tableno <= 0) {
        return kCommFaild;
    }

    return kCommSucc;
}
