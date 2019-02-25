#pragma once
#include "robot_define.h"
class RobotUtils {
public:
    static int SendRequestWithLock(CDefSocketClientPtr& connection, RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool need_echo = true);

    static CString ExecHttpRequestPost(const CString& url, const CString& params);

    static int GenRandInRange(int min_value, int max_value, int& random_result);

    static std::string GetGameIP();

    static int GetGamePort();

    static int IsValidGameID(GameID game_id);

    static int IsValidUserID(UserID userid);

    static int IsValidRoomID(RoomID roomid);

    static int IsValidTableNO(TableNO tableno);

    static int IsValidChairNO(ChairNO chairno);
};

#define CHECK_GAMEID(x)  if(kCommFaild == RobotUtils::IsValidGameID(x))  assert(false); return kCommFaild;

#define CHECK_USERID(x)  if(kCommFaild == RobotUtils::IsValidUserID(x))  assert(false); return kCommFaild;

#define CHECK_ROOMID(x)  if(kCommFaild == RobotUtils::IsValidRoomID(x))  assert(false); return kCommFaild;

#define CHECK_TABLENO(x)  if(kCommFaild == RobotUtils::IsValidTableNO(x))  assert(false); return kCommFaild;

#define CHECK_CHAIRNO(x)  if(kCommFaild == RobotUtils::IsValidChairNO(x))  assert(false); return kCommFaild;