#include "stdafx.h"
#include "user.h"
#include "robot_utils.h"


User::User() {}


User::~User() {}

int User::SnapShotObjectStatus() {
    LOG_FUNC("[SNAPSHOT] BEG");

    LOG_INFO("OBJECT ADDRESS [%x]", this);
    LOG_INFO("user_id_ [%d] user_type_ [%d] deposit_ [%d] room_id_ [%d] table_no_ [%d] chair_no_ [%d]", user_id_, user_type_, deposit_, room_id_, table_no_, chair_no_);

    LOG_FUNC("[SNAPSHOT] END");
    return kCommSucc;
}
