#include "stdafx.h"
#include "room.h"


Room::Room()
{
}


Room::~Room()
{
}

RoomOptional Room::GetRoomType()
{
	if (IS_BIT_SET(options_, kClassicsRoom)) {
		return kClassicsRoom;
	}
	else if (IS_BIT_SET(options_, kPrivateRoom)) {
		return kPrivateRoom;
	}
	else if (IS_BIT_SET(options_, kMatchRoom)) {
		return kMatchRoom;
	}
	else{
		return kUnkonwRoom;
	}
}
