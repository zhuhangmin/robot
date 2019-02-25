#pragma once
#include "user.h"
#include "table.h"
#include "robot_define.h"
enum RoomOptional {
    kUnkonwRoom = 0x00000000,	// 不存在的房间属性
    kClassicsRoom = 0x00000001,	// 经典房
    kPrivateRoom = 0x00000002,	// 私人房
    kMatchRoom = 0x00000003	// 比赛房
};

class BaseRoom {
public:
    BaseRoom();
    BaseRoom(int roomid);
    virtual ~BaseRoom();

    virtual RoomOptional GetRoomType();

    // 玩家进入房间(如果tableno>0，则加入指定桌子。 否则由服务端自动分配)
    virtual int PlayerEnterGame(const std::shared_ptr<User> &user);
    int BindPlayer(const std::shared_ptr<User> &user);
    // 旁观者进入房间
    virtual int LookerEnterGame(const std::shared_ptr<User> &user);
    // 玩家离开游戏
    virtual int UserLeaveGame(int userid, int tableno);
    int UnbindUser(int userid, int tableno);
    // 玩家弃牌
    virtual int UserGiveUp(int userid, int tableno);
    // 旁观者转玩家
    virtual int Looker2Player(std::shared_ptr<User> &user);
    // 玩家转旁观者
    virtual int Player2Looker(std::shared_ptr<User> &user);

    // 判断桌子号是否合法
    virtual bool IsValidTable(int tableno);
    // 判断银子数是否合法
    virtual bool IsValidDeposit(INT64 deposit);

public:
    int AddTable(TableNO tableno, std::shared_ptr<Table> table);

    // 获取桌子， 如果没有获取到table 返回的桌子号为0
    virtual std::shared_ptr<Table> GetTable(int tableno);

protected:
    // 获取符合要求的桌子号范围（用于缩小遍历桌子的范围）
    virtual int GetEligibleTable(const std::shared_ptr<User> &user, int &min_tableno, int &max_tableno);

private:
    std::array<std::shared_ptr<Table>, kMaxTableCountPerRoom> tables_;
    int room_id_ = 0;
    int options_ = 0;
    int configs_ = 0;
    int manages_ = 0;
    int max_table_cout_ = 0;			// 当前房间最大桌子数
    int min_playercount_per_table_ = 0;	// 满足开局条件的最小玩家数
    int chaircount_per_table_ = 0;		// 桌子上的椅子数

    INT64 min_deposit_ = 0;				// 进入房间的最小银子数
    INT64 max_deposit_ = 0;				// 进入房间的最大银子数


public:
    //////////////////////////////////////////////////////////////////////////

    int get_room_id() const {
        return room_id_;
    }
    void set_room_id(const int &val) {
        room_id_ = val;
    }

    int get_options() const {
        return options_;
    }
    void set_options(const int &val) {
        options_ = val;
    }

    int get_configs() const {
        return configs_;
    }
    void set_configs(const int &val) {
        configs_ = val;
    }

    int get_manages() const {
        return manages_;
    }
    void set_manages(const int &val) {
        manages_ = val;
    }

    int get_max_table_cout() const {
        return max_table_cout_;
    }
    void set_max_table_cout(const int &val) {
        if (val < kMinTableCountPerRoom) {
            max_table_cout_ = kMinTableCountPerRoom;
        } else if (val > kMaxTableCountPerRoom) {
            max_table_cout_ = kMaxTableCountPerRoom;
        } else {
            max_table_cout_ = val;
        }
    }

    int get_chaircount_per_table() const {
        return chaircount_per_table_;
    }
    void set_chaircount_per_table(const int &val) {
        if (val < kMinChairCountPerTable) {
            chaircount_per_table_ = kMinChairCountPerTable;
        } else if (val > kMaxChairCountPerTable) {
            chaircount_per_table_ = kMaxChairCountPerTable;
        } else {
            chaircount_per_table_ = val;
        }
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

    int get_min_playercount_per_table() const {
        return min_playercount_per_table_;
    }
    void set_min_playercount_per_table(const int &val) {
        min_playercount_per_table_ = val;
    }

};

