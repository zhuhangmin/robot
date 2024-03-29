#include "stdafx.h"
#include "user.h"
#include "robot_utils.h"

int User::SnapShotObjectStatus() const {
#ifdef _DEBUG
    LOG_INFO("userid [%d] type [0x%x][%s] roomid [%d] tableno [%d] chairno [%d] deposit [%I64d]",
             user_id_, user_type_, USER_TYPE_STR(user_type_), room_id_, table_no_, chair_no_, deposit_);
#endif
    return kCommSucc;
}
