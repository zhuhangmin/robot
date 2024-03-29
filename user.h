#pragma once

class User {

public:

    int get_token() const {
        return token_;
    }
    void set_token(const int &val) {
        token_ = val;
    }

    SOCKET get_sock() const {
        return sock_;
    }
    void set_sock(const SOCKET &val) {
        sock_ = val;
    }

    int get_user_id() const {
        return user_id_;
    }
    void set_user_id(const int &val) {
        user_id_ = val;
    }

    int get_user_type() const {
        return user_type_;
    }
    void set_user_type(const int &val) {
        user_type_ = val;
    }

    INT64 get_deposit() const {
        return deposit_;
    }
    void set_deposit(const INT64 &val) {
        deposit_ = val;
    }

    int get_room_id() const {
        return room_id_;
    }
    void set_room_id(const int &val) {
        room_id_ = val;
    }

    int get_table_no() const {
        return table_no_;
    }
    void set_table_no(const int &val) {
        table_no_ = val;
    }

    int get_chair_no() const {
        return chair_no_;
    }
    void set_chair_no(const int &val) {
        chair_no_ = val;
    }

    int get_offline_count() const {
        return offline_count_;
    }
    void set_offline_count(const int &val) {
        offline_count_ = val;
    }

    std::string get_head_url() const {
        return head_url_;
    }
    void set_head_url(const std::string &val) {
        head_url_ = val;
    }

    std::string get_hard_id() const {
        return hard_id_;
    }
    void set_hard_id(const std::string &val) {
        hard_id_ = val;
    }

    std::string get_nick_name() const {
        return nick_name_;
    }
    void set_nick_name(const std::string &val) {
        nick_name_ = val;
    }

    int get_win_bout() const {
        return win_bout_;
    }
    void set_win_bout(const int &val) {
        win_bout_ = val;
    }

    int get_total_bout() const {
        return total_bout_;
    }
    void set_total_bout(const int &val) {
        total_bout_ = val;
    }

    int get_loss_bout() const {
        return loss_bout_;
    }
    void set_loss_bout(const int &val) {
        loss_bout_ = val;
    }

    int get_standoff_bout() const {
        return standoff_bout_;
    }
    void set_standoff_bout(const int &val) {
        standoff_bout_ = val;
    }

    bool IsPalyer() const {
        return get_chair_no() > 0;
    }
public:
    // 对象状态快照
    int SnapShotObjectStatus() const;

private:
    int token_ = 0;
    SOCKET sock_ = 0;

    int user_id_ = 0;           // InvalidUserID
    int user_type_ = 0;			// 用户类型：机器人、管理员、普通玩家
    INT64 deposit_ = 0;			// 银子数

    int room_id_ = 0;
    int table_no_ = 0;
    int chair_no_ = 0;			// 有桌子号没有椅子号说明在旁观

    int offline_count_ = 0;

    std::string head_url_;
    std::string hard_id_;
    std::string nick_name_;

    int win_bout_ = 0;				// 赢的局数
    int total_bout_ = 0;			// 总局数
    int loss_bout_ = 0;				// 输的局数
    int standoff_bout_ = 0;			// 和的局数
};


using UserPtr = std::shared_ptr<User>;