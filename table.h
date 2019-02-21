#pragma once
#include "user.h"
#include "RobotDef.h"
class TableUserInfo {
public:
    TableUserInfo() {}
    ~TableUserInfo() {}

    int get_userid() const {
        return userid_;
    }
    void set_userid(const int &val) {
        userid_ = val;
    }

    INT64 get_bind_timestamp() const {
        return bind_timestamp_;
    }
    void set_bind_timestamp(const INT64 &val) {
        bind_timestamp_ = val;
    }

    int get_user_type() const {
        return user_type_;
    }
    void set_user_type(const int &val) {
        user_type_ = val;
    }

private:
    int userid_ = 0;
    int user_type_ = kUserNormal;	// �û�����
    INT64 bind_timestamp_ = 0;		// �û�������ʱ���
};

class ChairInfo {
public:
    ChairInfo() {}
    ~ChairInfo() {}

    int get_userid() const {
        return userid_;
    }
    void set_userid(const int &val) {
        userid_ = val;
    }

    ChairStatus get_chair_status() const {
        return chair_status_;
    }
    void set_chair_status(const ChairStatus &val) {
        chair_status_ = val;
    }

    /*int get_bind_timestamp() const {
        return bind_timestamp_;
        }
        void set_bind_timestamp(const int &val) {
        bind_timestamp_ = val;
        }*/
private:
    int userid_ = 0;
    //int bind_timestamp_ = 0;		// ��Һ����Ӱ󶨵�ʱ������룩
    ChairStatus chair_status_ = kChairWaiting;	// ����״̬
};

class UserResult {
public:
    UserResult() {}
    ~UserResult() {}

    void clear() {
        userid_ = 0;
        diff_deposit_ = 0;
        old_deposit_ = 0;
        time_cost_ = 0;
        bout_count_ = 0;
        win_count_ = 0;
        loss_count_ = 0;
        standoff_count_ = 0;
        offline_count_ = 0;
    }

public:
    int get_userid() const {
        return userid_;
    }
    void set_userid(const int &val) {
        userid_ = val;
    }

    INT64 get_diff_deposit() const {
        return diff_deposit_;
    }
    void set_diff_deposit(const INT64 &val) {
        diff_deposit_ = val;
    }

    INT64 get_old_deposit() const {
        return old_deposit_;
    }
    void set_old_deposit(const INT64 &val) {
        old_deposit_ = val;
    }

    int get_time_cost() const {
        return time_cost_;
    }
    void set_time_cost(const int &val) {
        time_cost_ = val;
    }

    int get_bout_count() const {
        return bout_count_;
    }
    void set_bout_count(const int &val) {
        bout_count_ = val;
    }

    int get_win_count() const {
        return win_count_;
    }
    void set_win_count(const int &val) {
        win_count_ = val;
    }

    int get_loss_count() const {
        return loss_count_;
    }
    void set_loss_count(const int &val) {
        loss_count_ = val;
    }

    int get_standoff_count() const {
        return standoff_count_;
    }
    void set_standoff_count(const int &val) {
        standoff_count_ = val;
    }

    int get_offline_count() const {
        return offline_count_;
    }
    void set_offline_count(const int &val) {
        offline_count_ = val;
    }
private:
    int userid_ = 0;
    INT64 diff_deposit_ = 0;	// ���ӱ仯
    INT64 old_deposit_ = 0;		// ԭ������������
    int time_cost_ = 0;			// ��Ϸ��ʱ���룩
    int bout_count_ = 0;		// һ�ν����� �ܾ���
    int win_count_ = 0;			// һ�ν����� Ӯ�ľ���
    int loss_count_ = 0;		// һ�ν����� ��ľ���
    int standoff_count_ = 0;	// һ�ν����� �͵ľ���
    int offline_count_ = 0;		// ���ߴ���
};


class Table {
public:
    Table();
    Table(int tableno, int roomid, int chair_count, int min_player_count, INT64 base_deposit);
    virtual ~Table();

    Table(const Table&) = delete;

    // ��Һ����ӽ��а�
    virtual int BindPlayer(const std::shared_ptr<User> &user, int &chairno);
    // �Թ��ߺ����ӽ��а�
    virtual void BindLooker(const std::shared_ptr<User> &user);
    // ����û������\�Թ��ߣ������ӵİ󶨹�ϵ
    virtual void UnbindUser(int userid);
    // �����Һ����ӵİ󶨹�ϵ
    virtual void UnbindPlayer(int userid);
    // ����Թ��ߺ����ӵİ󶨹�ϵ
    virtual void UnbindLooker(int userid);
public:

    // ��ȡ�������
    virtual int GetPlayerCount();
    // ��ȡָ�������ϵ�userid
    virtual int GetUserID(int chairno);
    // ��ȡ�����ϵĿ���λ����
    virtual int GetFreeChairCount();

    // �Ƿ�Ϊ���
    virtual bool IsTablePlayer(int userid);
    // �Ƿ�Ϊ�Թ���
    virtual bool IsTableLooker(int userid);
    // �û��Ƿ�������
    virtual bool IsTableUser(int userid);
    // ��ȡ�û������Ӻţ�������Ӻ���0����˵�����Թ���
    virtual int GetUserChair(int userid);
    // ��ȡ������Ϣ
    virtual ChairInfo GetChairInfoByChairno(int chairno);
    virtual ChairInfo GetChairInfoByUserid(int userid);
    // У�����Ӻ��Ƿ�Ϸ�
    virtual bool IsValidChairno(int chairno);
    // У���������Ƿ�Ϸ�
    virtual bool IsValidDeposit(INT64 deposit);

    // ������һ���û���ʱ����Ϸ�Ƿ��ܹ�����  TODO:�޸ĺ�����
    virtual bool IfContinueWhenOneUserLeave();

public:
    void AddChair(ChairNO chairno, ChairInfo info);
    void AddTableUserInfo(UserID userid, TableUserInfo table_user_info);
private:

    int table_no_ = 0;
    int room_id_ = 0;
    int chair_count_ = 0;			// ������
    int banker_chair_ = 0;			// ׯ�����Ӻ�
    int min_player_count_ = 0;		// ���ֵ���С�������

    std::array<ChairInfo, kMaxChairCountPerTable> chairs_{};		// ������Ϣ���±�Ϊ���Ӻ�-1
    std::hash_map<UserID, TableUserInfo> table_users_;					// �����������û� �����Թ���

    INT64 min_deposit_ = 0;			// ��С������
    INT64 max_deposit_ = 0;			// ���������
    INT64 base_deposit_ = 0;		// ������

    int table_status_ = kTableWaiting;	// ״̬�����TableStatus

    int countdown_timestamp_ = 0;	// ����ʱ��ʼʱ���
    int game_start_timestamp_ = 0;	// ��Ϸ��ʼʱ���

public:
    int get_table_no() const {
        return table_no_; //start from 1 
    }
    void set_table_no(const int &val) {
        table_no_ = val;
    }

    int get_room_id() const {
        return room_id_;
    }
    void set_room_id(const int &val) {
        room_id_ = val;
    }

    int get_chair_count() const {
        return chair_count_;
    }
    void set_chair_count(const int &val) {
        chair_count_ = val;
    }

    int get_banker_chair() const {
        return banker_chair_;
    }
    void set_banker_chair(const int &val) {
        banker_chair_ = val;
    }

    INT64 get_min_deposit() const {
        return min_deposit_;
    }
    void set_min_deposit(const INT64 &val) {
        min_deposit_ = val;
    }

    INT64 get_max_deposit() const {
        return max_deposit_;
    }
    void set_max_deposit(const INT64 &val) {
        max_deposit_ = val;
    }

    INT64 get_base_deposit() const {
        return base_deposit_;
    }
    void set_base_deposit(const INT64 &val) {
        base_deposit_ = val;
    }

    int get_table_status() const {
        return table_status_;
    }
    void set_table_status(const int &val) {
        table_status_ = val;
    }

    int get_min_player_count() const {
        return min_player_count_;
    }
    void set_min_player_count(const int &val) {
        min_player_count_ = val;
    }

    int get_countdown_timestamp() const {
        return countdown_timestamp_;
    }
    void set_countdown_timestamp(const int &val) {
        countdown_timestamp_ = val;
    }

    int get_game_start_timestamp() const {
        return game_start_timestamp_;
    }
    void set_game_start_timestamp(const int &val) {
        game_start_timestamp_ = val;
    }
};
