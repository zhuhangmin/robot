#include "stdafx.h"
#include "table.h"
#include "user.h"
#include "robot_utils.h"

Table::Table(const int& tableno, const int& roomid, const int& chair_count, const int& min_player_count, const INT64& base_deposit)
    :table_no_(tableno)
    , room_id_(roomid)
    , chair_count_(chair_count)
    , min_player_count_(min_player_count)
    , base_deposit_(base_deposit) {}


int Table::BindPlayer(const UserPtr &user) {
    CHECK_USER(user);
    const auto userid = user->get_user_id();
    const auto chairno = user->get_chair_no();
    chairs_[chairno].set_userid(userid);
    chairs_[chairno].set_chair_status(kChairWaiting);

    TableUserInfo userinfo;
    userinfo.set_userid(userid);
    userinfo.set_user_type(user->get_user_type());
    table_users_[userid] = userinfo;

    return kCommSucc;
}

int Table::BindLooker(const UserPtr &user) {
    CHECK_USER(user);
    TableUserInfo userinfo;
    userinfo.set_userid(user->get_user_id());
    userinfo.set_user_type(user->get_user_type());
    table_users_[user->get_user_id()] = userinfo;
    return kCommSucc;
}

int Table::UnbindUser(const int& userid) {
    CHECK_USERID(userid);
    if (kCommSucc != UnbindPlayer(userid)) {
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != UnbindLooker(userid)) {
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int Table::UnbindPlayer(const int& userid) {
    CHECK_USERID(userid);
    const auto chairno = GetUserChair(userid);
    CHECK_CHAIRNO(chairno);
    chairs_[chairno - 1].set_userid(0);
    chairs_[chairno - 1].set_chair_status(kChairWaiting);
    return kCommSucc;
}

int Table::UnbindLooker(const int& userid) {
    CHECK_USERID(userid);
    table_users_.erase(userid);
    return kCommSucc;
}

int Table::GetPlayerCount() {
    auto player_count = 0;

    for (const auto &chair_info : chairs_) {
        if (chair_info.get_userid() > 0) {
            ++player_count;
        }
    }

    return player_count;
}

int Table::GetUserID(const int& chairno) {
    CHECK_CHAIRNO(chairno);
    const auto index = chairno - 1;
    if (index < 0 || index >= chairs_.size()) {
        return 0;
    }
    return chairs_.at(index).get_userid();
}

int Table::GetFreeChairCount() {
    for (auto i = 0; i < get_chair_count(); ++i) {
        if (get_chair_count() - GetPlayerCount() == i) {
            return i;
        }
    }
    return 0;
}

bool Table::IsTablePlayer(const int& userid) {
    const auto chairno = GetUserChair(userid);
    return IsValidChairno(chairno);
}

bool Table::IsTableLooker(const int& userid) {
    const auto chairno = GetUserChair(userid);

    if (!IsTableUser(userid)) return false;

    if (IsValidChairno(chairno)) return false;

    return false;
}

bool Table::IsTableUser(const int& userid) {
    return table_users_.find(userid) != table_users_.end();
}

int Table::GetUserChair(const int& userid) {
    CHECK_USERID(userid);
    for (auto i = 0; i < get_chair_count() && i < chairs_.size(); ++i) {
        if (chairs_.at(i).get_userid() == userid) {
            // �±�+1�������Ӻ�
            return i + 1;
        }
    }
    return 0;
}

int Table::GetChairInfoByChairno(const int& chairno, ChairInfo& info) {
    CHECK_CHAIRNO(chairno);
    if (!IsValidChairno(chairno)) {
        LOG_ERROR("Invalid chairno[%d]", chairno);
        ASSERT_FALSE_RETURN;
    }

    info = chairs_.at(chairno - 1);
    return kCommSucc;
}

int Table::GetChairInfoByUserid(const int& userid, ChairInfo& info) {
    CHECK_USERID(userid);
    const auto chairno = GetUserChair(userid);
    if (kCommSucc != GetChairInfoByChairno(chairno, info)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

bool Table::IsValidChairno(const int& chairno) {
    if (chairno > 0 && chairno <= get_chair_count() && chairno <= chairs_.size()) {
        return true;
    }
    return false;
}

bool Table::IsValidDeposit(const INT64& deposit) {
    if (get_min_deposit() == 0 && get_max_deposit() == 0) {
        return true;
    }

    return deposit >= get_min_deposit() && deposit < get_max_deposit();
}


bool Table::IfContinueWhenOneUserLeave() {
    return GetPlayerCount() - 1 >= get_min_player_count();
}


int Table::AddChair(const ChairNO& chairno, const ChairInfo& info) {
    CHECK_CHAIRNO(chairno);
    chairs_[chairno - 1] = info;
    return kCommSucc;
}

int Table::AddTableUserInfo(const UserID& userid, const TableUserInfo& table_user_info) {
    CHECK_USERID(userid);
    table_users_[userid] = table_user_info;
    return kCommSucc;
}

int Table::GiveUp(const int& userid) {
    CHECK_USERID(userid);
    if (!IS_BIT_SET(get_table_status(), kTablePlaying)) {
        LOG_WARN("user[%d] giveup, but not playing.", userid);
        ASSERT_FALSE_RETURN;
    }

    // ����
    if (IfContinueWhenOneUserLeave()) {
        return RefreshGameResult(userid);
    }

    return RefreshGameResult();
}

int Table::RefreshGameResult() {
    // �޸�����״̬
    set_table_status(kTableWaiting);
    for (auto &chair_info : chairs_) {
        chair_info.set_chair_status(kChairWaiting);
    }

    return kCommSucc;
}

int Table::RefreshGameResult(const int& userid) {
    CHECK_USERID(userid);
    const auto chairno = GetUserChair(userid);
    if (!IsValidChairno(chairno)) {
        LOG_WARN("user[%d] is not player.", userid);
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int Table::SnapShotObjectStatus() {
    LOG_INFO("OBJECT ADDRESS [%x]", this);
    LOG_INFO("table_no_ [%d] table_status_ [%d] chair_count_ [%d] banker_chair_ [%d] base_deposit_ [%d] room_id_ [%d] ", table_no_, table_status_, chair_count_, banker_chair_, base_deposit_, room_id_);

    LOG_INFO("chairs_ size [%d]", chairs_.size());
    for (auto& chairinfo : chairs_) {
        LOG_INFO("chair userid [%d] status [%d]", chairinfo.get_userid(), chairinfo.get_chair_status());
    }

    LOG_INFO("table_users_ size [%d]", table_users_.size());
    for (auto& kv: table_users_) {
        auto tableinfo = kv.second;
        LOG_INFO("table userid [%d] user_type_ [%d] bind_timestamp_ [%d]", tableinfo.get_userid(), tableinfo.get_user_type(), tableinfo.get_bind_timestamp());
    }

    return kCommSucc;
}