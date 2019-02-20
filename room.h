#pragma once

enum RoomOptional
{
	kUnkonwRoom		= 0x00000000,	// 不存在的房间属性
	kClassicsRoom	= 0x00000001,	// 经典房
	kPrivateRoom	= 0x00000002,	// 私人房
	kMatchRoom		= 0x00000003	// 比赛房
};

class Room
{
public:
	Room();
	virtual ~Room();

	RoomOptional GetRoomType();

	
private:

	std::mutex tables_mutex_;
	std::hash_map<int, Table> tables_;

	int options_ = 0;
	int configs_ = 0;
	int manages_ = 0;
	int max_table_cout_ = 0;
	int chaircount_per_table_ = 0;

	INT64 min_deposit_ = 0;
	INT64 max_deposit_ = 0;

public:
	//////////////////////////////////////////////////////////////////////////
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
		max_table_cout_ = val;
	}

	int get_chaircount_per_table() const {
		return chaircount_per_table_;
	}
	void set_chaircount_per_table(const int &val) {
		chaircount_per_table_ = val;
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
};

class PrivateRoom : public Room
{
public:
	PrivateRoom();
	~PrivateRoom();

private:

};

PrivateRoom::PrivateRoom()
{
}

PrivateRoom::~PrivateRoom()
{
}

