#pragma once
#include "user.h"
#include "robot_define.h"
class TableUserInfo {
public:
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

    INT64 get_offline_timestamp() const {
        return offline_timestamp_;
    }
    void set_offline_timestamp(const INT64 &val) {
        offline_timestamp_ = val;
    }

private:
    int userid_ = 0;
    int user_type_ = kUserNormal;	// 用户类型
    INT64 bind_timestamp_ = 0;		// 用户上桌的时间戳
    INT64 offline_timestamp_ = 0;	// 用户离线的时间戳，0表示在线
};

class ChairInfo {
public:
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

    int get_bind_timestamp() const {
        return bind_timestamp_;
    }
    void set_bind_timestamp(const int &val) {
        if (val < 0) assert(false);
        bind_timestamp_ = val;
    }

    bool get_player2looker_next_time() const {
        return player2looker_next_time_;
    }
    void set_player2looker_next_time(const bool &val) {
        player2looker_next_time_ = val;
    }

    int get_chair_no() const {
        return chairno_;
    }

    void set_chair_no(const int &val) {
        chairno_ = val;
    }

private:
    int chairno_ = 0;
    int userid_ = 0;
    int bind_timestamp_ = 0;					// 玩家和椅子绑定的时间戳（秒）
    ChairStatus chair_status_ = kChairWaiting;	// 椅子状态
    bool player2looker_next_time_ = false;		// 是否需要在下局站起（变成旁观玩家）
};

class UserResult {
public:
    void clear() {
        userid_ = 0;
        diff_deposit_ = 0;
        old_deposit_ = 0;
        set_fee(0);
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

    INT64 get_fee() const {
        return fee_;
    }
    void set_fee(const INT64 &val) {
        fee_ = val;
    }
private:
    int userid_ = 0;
    INT64 diff_deposit_ = 0;	// 银子变化
    INT64 old_deposit_ = 0;		// 原来持有银子数
    INT64 fee_ = 0;				// 茶水费
    int time_cost_ = 0;			// 游戏耗时（秒）
    int bout_count_ = 0;		// 一次结算中 总局数
    int win_count_ = 0;			// 一次结算中 赢的局数
    int loss_count_ = 0;		// 一次结算中 输的局数
    int standoff_count_ = 0;	// 一次结算中 和的局数
    int offline_count_ = 0;		// 掉线次数（暂时用不到）
};


class Table {
public:
    Table() {};

    Table(const int& tableno, const int& roomid, const int& chair_count, const int& min_player_count, const INT64& base_deposit);

    Table(const Table&) = delete;

    // 玩家和桌子进行绑定
    virtual int BindPlayer(const UserPtr &user);
    // 旁观者和桌子进行绑定
    int BindLooker(const UserPtr &user);
    // 解除用户（玩家\旁观者）和桌子的绑定关系
    int UnbindUser(const int& user);
    // 解除玩家和桌子的绑定关系
    int UnbindPlayer(const int& userid);
    // 解除旁观者和桌子的绑定关系
    int UnbindLooker(const int& userid);
    // 开始游戏
    virtual int StartGame();
    // (游戏中)弃牌
    virtual int GiveUp(const int& userid);
    // 整桌结算
    virtual int RefreshGameResult();
    // 单人结算
    virtual int RefreshGameResult(const int& userid);

public:

    // 获取玩家数量
    virtual int GetPlayerCount();
    // 获取机器人数量
    virtual int GetRobotCount(int& count);
    // 获取指定椅子上的userid
    virtual int GetUserID(const int& chairno);
    // 获取桌子上的空座位数量
    virtual int GetFreeChairCount();

    // 是否为玩家
    virtual bool IsTablePlayer(const int& userid);
    // 是否为旁观者
    virtual bool IsTableLooker(const int& userid);
    // 用户是否在桌上
    virtual bool IsTableUser(const int& userid);
    // 获取用户的椅子号，如果椅子号是0，则说明是旁观者
    virtual int GetUserChair(const int& userid);
    // 获取椅子信息
    int GetChairInfoByChairno(const int& chairno, ChairInfo& info);
    int GetChairInfoByUserid(const int& userid, ChairInfo& info);
    // 校验椅子号是否合法
    virtual bool IsValidChairno(const int& chairno);
    // 校验银子数是否合法
    virtual bool IsValidDeposit(const INT64& deposit);

    // 当减少一个用户的时候游戏是否能够继续 
    virtual bool IfContinueWhenOneUserLeave();

public:
    int AddChair(const ChairNO& chairno, const ChairInfo& info);
    int AddTableUserInfo(const UserID& userid, const TableUserInfo& table_user_info);
    std::array<ChairInfo, kMaxChairCountPerTable>& GetChairs() { return chairs_; }

public:
    // 对象状态快照
    int SnapShotObjectStatus();

private:

    int table_no_ = 0;
    int room_id_ = 0;
    int chair_count_ = 0;			// 椅子数
    int banker_chair_ = 0;			// 庄家椅子号
    int min_player_count_ = 0;		// 开局的最小玩家数量

    std::array<ChairInfo, kMaxChairCountPerTable> chairs_ = {};		// 椅子信息，下标为椅子号-1
    std::array<UserResult, kMaxChairCountPerTable> user_results_ = {};	// 桌上玩家结果信息，下标为椅子号-1
    std::vector<UserResult> commited_results_ = {};					// 已提交的结果信息（用户弃牌，自行结算后吧结算数据保存，等全桌结算的时候需要用到此信息）
    std::hash_map<int, TableUserInfo> table_users_;					// 桌子上所有用户 包括旁观者

    INT64 min_deposit_ = 0;			// 最小银子数
    INT64 max_deposit_ = 0;			// 最大银子数
    INT64 base_deposit_ = 0;		// 基础银

    int table_status_ = kTableWaiting;	// 状态定义见TableStatus

    int countdown_time_s_ = 5;		// 游戏开始 倒计时时间（默认5秒）
    int countdown_timestamp_ = 0;	// 倒计时开始时间戳
    int game_start_timestamp_ = 0;	// 游戏开始时间戳

public:
    int get_table_no() const {
        return table_no_;
    }

    int set_table_no(const int &val) {
        return table_no_ = val;
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

    int get_countdown_time_s() const {
        return countdown_time_s_;
    }
    void set_countdown_time_s(const int &val) {
        countdown_time_s_ = val;
    }
};

using TablePtr = std::shared_ptr<Table>;