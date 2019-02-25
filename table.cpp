#include "stdafx.h"
#include "table.h"
#include "user.h"
#include "robot_utils.h"

Table::Table() {}


Table::Table(int tableno, int roomid, int chair_count, int min_player_count, INT64 base_deposit)
    :table_no_(tableno)
    , room_id_(roomid)
    , chair_count_(chair_count)
    , min_player_count_(min_player_count)
    , base_deposit_(base_deposit) {}

Table::~Table() {}

int Table::BindPlayer(const std::shared_ptr<User> &user) {
    int userid = user->get_user_id();
    int chairno = user->get_chair_no();
    chairs_[chairno].set_userid(userid);
    chairs_[chairno].set_chair_status(kChairWaiting);

    TableUserInfo userinfo;
    userinfo.set_userid(userid);
    userinfo.set_user_type(user->get_user_type());
    table_users_[userid] = userinfo;

    return kCommSucc;
}

void Table::BindLooker(const std::shared_ptr<User> &user) {
    TableUserInfo userinfo;
    userinfo.set_userid(user->get_user_id());
    userinfo.set_user_type(user->get_user_type());
    table_users_[user->get_user_id()] = userinfo;
}

void Table::UnbindUser(int userid) {
    UnbindPlayer(userid);
    UnbindLooker(userid);
}

void Table::UnbindPlayer(int userid) {
    int chairno = GetUserChair(userid);
    assert(chairno >= 1);
    chairs_[chairno - 1].set_userid(0);
    chairs_[chairno - 1].set_chair_status(kChairWaiting);
}

void Table::UnbindLooker(int userid) {
    table_users_.erase(userid);
}

int Table::GetPlayerCount() {
    int player_count = 0;

    for (const auto &chair_info : chairs_) {
        if (chair_info.get_userid() > 0) {
            ++player_count;
        }
    }

    return player_count;
}

int Table::GetUserID(int chairno) {
    int index = chairno - 1;
    if (index < 0 || index >= chairs_.size()) {
        return 0;
    }
    return chairs_.at(index).get_userid();
}

int Table::GetFreeChairCount() {
    for (int i = 0; i < get_chair_count(); ++i) {
        if (get_chair_count() - GetPlayerCount() == i) {
            return i;
        }
    }
    return 0;
}

bool Table::IsTablePlayer(int userid) {
    int chairno = GetUserChair(userid);
    return IsValidChairno(chairno);
}

bool Table::IsTableLooker(int userid) {
    int chairno = GetUserChair(userid);
    if (IsTableUser(userid) && !IsValidChairno(chairno)) {
        return true;
    }
    return false;
}

bool Table::IsTableUser(int userid) {
    auto itr = table_users_.find(userid);
    if (itr == table_users_.end()) {
        return false;
    }
    return true;
}

int Table::GetUserChair(int userid) {
    for (int i = 0; i < get_chair_count() && i < chairs_.size(); ++i) {
        if (chairs_.at(i).get_userid() == userid) {
            // 下标+1就是椅子号
            return i + 1;
        }
    }
    return 0;
}

ChairInfo Table::GetChairInfoByChairno(int chairno) {
    if (IsValidChairno(chairno)) {
        UWL_WRN("Invalid chairno[%d]", chairno);
        ChairInfo info;
        return info;
    }

    return chairs_.at(chairno - 1);
}

ChairInfo Table::GetChairInfoByUserid(int userid) {
    int chairno = GetUserChair(userid);
    return GetChairInfoByChairno(chairno);
}

bool Table::IsValidChairno(int chairno) {
    if (chairno > 0 && chairno <= get_chair_count() && chairno <= chairs_.size()) {
        return true;
    }
    return false;
}

bool Table::IsValidDeposit(INT64 deposit) {
    if (get_min_deposit() == 0 && get_max_deposit() == 0) {
        return true;
    }

    return (deposit >= get_min_deposit() && deposit < get_max_deposit());
}


bool Table::IfContinueWhenOneUserLeave() {
    return GetPlayerCount() - 1 >= get_min_player_count();
}


int Table::AddChair(ChairNO chairno, ChairInfo info) {
    CHECK_CHAIRNO(chairno);
    chairs_[chairno - 1] = info;
}

int Table::AddTableUserInfo(UserID userid, TableUserInfo table_user_info) {
    CHECK_USERID(userid);
    table_users_[userid] = table_user_info;
    return kCommSucc;
}

int Table::GiveUp(int userid) {
    if (!IS_BIT_SET(get_table_status(), kTablePlaying)) {
        UWL_WRN("user[%d] giveup, but not playing.", userid);
        return kCommFaild;
    }

    // 结算
    if (IfContinueWhenOneUserLeave()) {
        return RefreshGameResult(userid);
    } else {
        return RefreshGameResult();
    }
}

int Table::RefreshGameResult() {
    // 修改桌椅状态
    set_table_status(kTableWaiting);
    for (auto &chair_info : chairs_) {
        chair_info.set_chair_status(kChairWaiting);
    }

    return kCommSucc;
}

int Table::RefreshGameResult(int userid) {
    int chairno = GetUserChair(userid);
    if (!IsValidChairno(chairno)) {
        UWL_WRN("user[%d] is not player.", userid);
        return kCommFaild;
    }

    return kCommSucc;
}
