#include "stdafx.h"
#include "RoomMgr.h"


RoomMgr::RoomMgr()
{
}


RoomMgr::~RoomMgr()
{
}

std::shared_ptr<BaseRoom> RoomMgr::NewRoom(int roomid/* = 0*/)
{
	return std::make_shared<BaseRoom>(roomid);
}

std::shared_ptr<BaseRoom> RoomMgr::GetRoom(int roomid)
{
	static std::shared_ptr<BaseRoom> null_room = std::make_shared<BaseRoom>();

	std::lock_guard<std::mutex> lock_g(rooms_mutex_);
	auto itr = rooms_.find(roomid);
	if (itr == rooms_.end()) {
		return null_room;
	}
	return itr->second;
}

void RoomMgr::AddRoom(int roomid, const std::shared_ptr<BaseRoom> &room)
{
	std::lock_guard<std::mutex> lock_g(rooms_mutex_);
	rooms_[roomid] = room;
}

std::hash_map<int, std::shared_ptr<BaseRoom>> RoomMgr::GetAllRooms()
{
	std::lock_guard<std::mutex> lock_g(rooms_mutex_);
	return rooms_;
}
