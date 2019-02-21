#pragma once
#include "user.h"
#include "table.h"
#include "RobotDef.h"
enum RoomOptional {
    kUnkonwRoom = 0x00000000,	// �����ڵķ�������
    kClassicsRoom = 0x00000001,	// ���䷿
    kPrivateRoom = 0x00000002,	// ˽�˷�
    kMatchRoom = 0x00000003	// ������
};

class BaseRoom {
public:
    BaseRoom();
    BaseRoom(int roomid);
    virtual ~BaseRoom();

    virtual RoomOptional GetRoomType();

    // ��ҽ��뷿��
    virtual int PlayerEnterGame(const std::shared_ptr<User> &user);
    // �Թ��߽��뷿��
    virtual int LookerEnterGame(const std::shared_ptr<User> &user, int target_tableno);
    // ��һط��� ��������
    virtual int ContinueGame(const std::shared_ptr<User> &user);
    // ����뿪��Ϸ
    virtual int UserLeaveGame(int userid, int tableno);
    // �������
    virtual int UserGiveUp(int userid, int tableno);
    // �Թ���ת���
    virtual int Looker2Player(std::shared_ptr<User> &user);
    // ���ת�Թ���
    virtual int Player2Looker(std::shared_ptr<User> &user);
    // ��һ���
    virtual int SwitchTable(std::shared_ptr<User> &user);

    // �ж����Ӻ��Ƿ�Ϸ�
    virtual bool IsValidTable(int tableno);
    // �ж��������Ƿ�Ϸ�
    virtual bool IsValidDeposit(INT64 deposit);

public:
    void AddTable(TableNO tableno, std::shared_ptr<Table> table);
protected:

    // ��ȡ���ӣ� ���û�л�ȡ��table ���ص����Ӻ�Ϊ0
    virtual std::shared_ptr<Table> GetTable(int tableno);

    // ��ȡ��������. �������Ӻ� shielding_tableno��ʾ��Ҫ���ε����Ӻ�
    virtual int GetAvailableTableno(const std::shared_ptr<User> &user, int shielding_tableno = 0);
    // ��ȡ����Ҫ������Ӻŷ�Χ��������С�������ӵķ�Χ��
    virtual int GetEligibleTable(const std::shared_ptr<User> &user, int &min_tableno, int &max_tableno);


    // ������������������
    virtual int SetUserTableInfo(int userid, int tableno, int chairno);

private:
    std::array<std::shared_ptr<Table>, kMaxTableCountPerRoom> tables_;

    std::mutex alloc_table_chair_mutex_;				// ����������  ��������������������ķ�Χ��ʹ�ã������������

    int room_id_ = 0;
    int options_ = 0;
    int configs_ = 0;
    int manages_ = 0;
    int max_table_cout_ = 0;			// ��ǰ�������������
    int min_playercount_per_table_ = 0;	// ���㿪����������С�����
    int chaircount_per_table_ = 0;		// �����ϵ�������

    INT64 min_deposit_ = 0;				// ���뷿�����С������
    INT64 max_deposit_ = 0;				// ���뷿������������
    INT64 base_deposit_ = 0;			// ������


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

    INT64 get_base_deposit() const {
        return base_deposit_;
    }
    void set_base_deposit(const INT64 &val) {
        base_deposit_ = val;
    }
};

