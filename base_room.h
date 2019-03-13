#pragma once
#include "user.h"
#include "table.h"
#include "robot_define.h"
enum RoomOptional {
    kUnkonwRoom = 0x00000000,	// �����ڵķ�������
    kClassicsRoom = 0x00000001,	// ���䷿
    kPrivateRoom = 0x00000002,	// ˽�˷�
    kMatchRoom = 0x00000003	// ������
};

class BaseRoom {
public:
    BaseRoom();
    explicit BaseRoom(const int& roomid);
    virtual ~BaseRoom();

    virtual RoomOptional GetRoomType();

    // ��ҽ��뷿��(���tableno>0�������ָ�����ӡ� �����ɷ�����Զ�����)
    virtual int PlayerEnterGame(const UserPtr &user);
    int BindPlayer(const UserPtr &user);
    // �Թ��߽��뷿��
    virtual int LookerEnterGame(const UserPtr &user);
    // ����뿪��Ϸ
    virtual int UserLeaveGame(const UserID& userid, const TableNO& tableno);
    int UnbindUser(const UserID& userid, const TableNO& tableno);
    // �������
    virtual int UserGiveUp(const UserID& userid, const TableNO& tableno);
    // �Թ���ת���
    virtual int Looker2Player(const UserPtr& user);
    // ���ת�Թ���
    virtual int Player2Looker(const UserPtr& user);
    // ��һ���
    virtual int SwitchTable(const UserPtr& user, const TableNO& tableno);
    // ��ʼ��Ϸ
    virtual int StartGame(const TableNO& tableno);
    // �ж����Ӻ��Ƿ�Ϸ�
    virtual bool IsValidTable(const TableNO& tableno) const;
    // �ж��������Ƿ�Ϸ�
    virtual bool IsValidDeposit(const INT64& deposit) const;

public:
    int AddTable(const TableNO& tableno, const TablePtr& table);

    // ��ȡ���ӣ� ���û�л�ȡ��table ���ص����Ӻ�Ϊ0
    int GetTable(const TableNO& tableno, TablePtr& table) const;

public:
    // ����״̬����
    int SnapShotObjectStatus();

private:
    std::array<TablePtr, kMaxTableCountPerRoom> tables_;
    RoomID room_id_ = 0;
    int options_ = 0;
    int configs_ = 0;
    int manages_ = 0;
    int max_table_cout_ = 0;			// ��ǰ�������������
    int min_playercount_per_table_ = 0;	// ���㿪����������С�����
    int chaircount_per_table_ = 0;		// �����ϵ�������

    INT64 min_deposit_ = 0;				// ���뷿�����С������
    INT64 max_deposit_ = 0;				// ���뷿������������


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
        //min_playercount_per_table_ = val;
        //TODO 

        min_playercount_per_table_ = 2;

    }

};


using RoomPtr = std::shared_ptr<BaseRoom>;