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

    // ��ȡһ����Ч����
    int tableno = GetAvailableTableno(user);

    // �������
    std::shared_ptr<Table> table = GetTable(tableno);
    if (!IsValidTable(table->get_table_no())) {
        UWL_WRN("Get table[%d] faild", tableno);
        return kCommFaild;
    }

    int chairno = 0;
    int ret = table->BindPlayer(user, chairno);
    if (ret != kCommSucc) {
        UWL_WRN("BindPlayer faild. userid=%d, tableno=%d", user->get_user_id(), tableno);
        return kCommFaild;
    }


    // ��ȫMakeUserInfoû�и�ֵ���ֶ�
    user->set_table_no(tableno);
    user->set_chair_no(chairno);
    user->set_enter_timestamp(time(0));


    UserMgr::Instance().AddUser(user->get_user_id(), user);

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

int BaseRoom::UserGiveUp(int userid, int tableno) {
    auto table = GetTable(tableno);
    if (!IsValidTable(table->get_table_no())) {
        UWL_WRN("Get table[%d] faild", tableno);
        return kCommFaild;
    }

    return kCommSucc;
}

// TODO:ȷ��response��code��ͨ������ֵ���ػ���getlasterror����
int BaseRoom::Looker2Player(std::shared_ptr<User> &user) {
    if (!IsValidDeposit(user->get_deposit())) {
        UWL_WRN("user[%d] deposit[%I64d] invalid.", user->get_user_id(), user->get_deposit());
        return kCommFaild;
    }

    auto table = GetTable(user->get_table_no());
    if (!IsValidTable(table->get_table_no())) {
        UWL_WRN("Get table[%d] faild", user->get_table_no());
        return kCommFaild;
    }

    // ����������
    std::lock_guard<std::mutex> lock_g(alloc_table_chair_mutex_);

    // ������

    if (!table->IsTableUser(user->get_user_id())) {
        UWL_WRN("user[%d] is not in table[%d]", user->get_user_id(), user->get_table_no());
        return kCommFaild;
    }

    if (table->IsTablePlayer(user->get_user_id())) {
        UWL_WRN("Looker2Player succ. but user[%d] is player.", user->get_user_id());
        // ����������ֱ�ӷ��سɹ�
        user->set_chair_no(table->GetUserChair(user->get_user_id()));
        return kCommSucc;
    }

    int chairno = 0;
    int ret = table->BindPlayer(user, chairno);
    if (ret != kCommSucc) {
        UWL_WRN("user[%d] BindPlayer faild. ret=%d", user->get_user_id(), ret);
        return kCommFaild;
    }

    user->set_chair_no(chairno);
    return kCommSucc;
}

int BaseRoom::Player2Looker(std::shared_ptr<User> &user) {
    auto table = GetTable(user->get_table_no());
    if (!IsValidTable(table->get_table_no())) {
        UWL_WRN("Get table[%d] faild", user->get_table_no());
        return kCommFaild;
    }

    // ����������(�������Ҳ���ᵼ�������������ʧ�ܣ������������)
    //std::lock_guard<std::mutex> lock_g(alloc_table_chair_mutex_);


    if (!table->IsTableUser(user->get_user_id())) {
        UWL_WRN("user[%d] is not in table[%d]", user->get_user_id(), user->get_table_no());
        return kCommFaild;
    }

    table->UnbindPlayer(user->get_user_id());

    // TODO:�޸�������Ӻ�Ϊ0

    return kCommSucc;
}

int BaseRoom::SwitchTable(std::shared_ptr<User> &user) {
    // �����ԭ���ӵĹ�ϵ
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
    }

    // ��������
    {
        std::lock_guard<std::mutex> lock_g(alloc_table_chair_mutex_);

        // ��ȡһ����Ч����
        int tableno = GetAvailableTableno(user, user->get_table_no());

        // �������
        auto table = GetTable(tableno);
        if (!IsValidTable(table->get_table_no())) {
            UWL_WRN("Get table[%d] faild", tableno);
            return kCommFaild;
        }

        int chairno = 0;
        int ret = table->BindPlayer(user, chairno);
        if (ret != kCommSucc) {
            UWL_WRN("BindPlayer faild. user[%d], table[%d]", user->get_user_id(), tableno);
            return kCommFaild;
        }

        // �����������Ϣ  TODO:Ҫ����usermanager�����user
        user->set_table_no(tableno);
        user->set_chair_no(chairno);
        user->set_enter_timestamp(time(0));
    }

    return kCommSucc;
}

int BaseRoom::LookerEnterGame(const std::shared_ptr<User> &user, int target_tableno) {
    std::shared_ptr<Table> table = GetTable(target_tableno);
    if (false == IsValidTable(table->get_table_no())) {
        UWL_WRN("Get table[%d] faild", target_tableno);
        return kCommFaild;
    }

    table->BindLooker(user);

    // ��ȫMakeUserInfoû�и�ֵ���ֶ�
    user->set_table_no(target_tableno);
    user->set_enter_timestamp(time(0));

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

    // ���Ŵ�1��ʼ���±��0��ʼ
    return tables_.at(tableno - 1);
}

int BaseRoom::GetAvailableTableno(const std::shared_ptr<User> &user, int shielding_tableno/* = 0*/) {
    const int faild_tableno = 0;
    int min_tableno = 0, max_tableno = 0;
    int ret = GetEligibleTable(user, min_tableno, max_tableno);
    if (ret != kCommSucc) {
        UWL_WRN("GetEligibleTable faild.");
        return faild_tableno;
    }

    // �±�Ϊ���ӵĿ���λ����valueΪ���Ӻ�
    std::array<int, kMaxChairCountPerTable + 1> available_tables = {};

    // ɨ�跶Χ���������ӣ��ҳ���ͬ���ȼ��Ŀ������Ӻ�
    for (int tableno = min_tableno; tableno < max_tableno; ++tableno) {
        if (tableno == shielding_tableno) {
            continue;
        }

        auto table = GetTable(tableno);
        if (!IsValidTable(table->get_table_no())) {
            continue;
        }


        if (!IsValidDeposit(user->get_deposit())) {
            continue;
        }

        int free_chair_count = table->GetFreeChairCount();
        if (free_chair_count <= 0 || free_chair_count > kMaxChairCountPerTable || free_chair_count > chaircount_per_table_) {
            UWL_WRN("GetFreeChairCount return [%d]", free_chair_count);
            continue;
        }

        available_tables.at(free_chair_count) = tableno;

        // �ҵ�ֻ��һ������λ�����ӾͲ������ˣ�ֱ������
        if (1 == free_chair_count) {
            break;
        }
    }

    // ѡ�����շ������ҵ�����
    for (int i = 0; i < available_tables.size(); ++i) {
        if (available_tables.at(i) <= 0) {
            continue;
        }

        return available_tables.at(i);
    }

    return faild_tableno;
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

int BaseRoom::SetUserTableInfo(int userid, int tableno, int chairno) {
    auto user = UserMgr::Instance().GetUser(userid);
    if (user->get_user_id() <= 0) {
        UWL_WRN("SetUserTableInfo get user[%d] faild.", userid);
        return kCommFaild;
    }

    user->set_table_no(tableno);
    user->set_chair_no(chairno);
    user->set_enter_timestamp(time(0));
    return kCommSucc;
}