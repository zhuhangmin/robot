#pragma once
#include "robot_define.h"
#include "user.h"
#include "table.h"
#include "base_room.h"
#include "robot_game.h"
class RobotUtils {
public:
    // 某个特定的connection发送协议
    static int SendRequestWithLock(CDefSocketClientPtr& connection, RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool need_echo = true);

    // 发送HTTP POST协议
    static CString ExecHttpRequestPost(const CString& url, const CString& params);

    // 产生随机数
    static int GenRandInRange(int min_value, int max_value, int& random_result);

    // 获得配置游戏服务器地址
    static std::string GetGameIP();

    // 获得动态游戏服务器端口
    static int GetGamePort();

    static int IsValidGameID(const GameID game_id);

    static int IsValidUserID(const UserID userid);

    static int IsValidRoomID(const RoomID roomid);

    static int IsValidTableNO(const TableNO tableno);

    static int IsValidChairNO(const ChairNO chairno);

    static int IsValidTokenID(const TokenID tokenid);

    static int IsValidRequestID(const RequestID requestid);

    static int IsValidUser(const UserPtr& user);

    static int IsValidTable(const TablePtr& table);

    static int IsValidRoom(const RoomPtr& room);

    static int IsValidRobot(const RobotPtr& robot);

    static int IsValidGameIP(const std::string& game_ip);

    static int IsValidGamePort(const int32_t game_port);

};

#define CHECK_GAMEID(x) if(kCommSucc != RobotUtils::IsValidGameID(x))  {assert(false); return kCommFaild;}

#define CHECK_USERID(x)  if(kCommSucc != RobotUtils::IsValidUserID(x))  {assert(false); return kCommFaild;}

#define CHECK_ROOMID(x)  if(kCommSucc != RobotUtils::IsValidRoomID(x)) { assert(false); return kCommFaild;}

#define CHECK_TABLENO(x)  if(kCommSucc != RobotUtils::IsValidTableNO(x))  {assert(false); return kCommFaild;}

#define CHECK_CHAIRNO(x)  if(kCommSucc != RobotUtils::IsValidChairNO(x))  {assert(false); return kCommFaild;}

#define CHECK_TOKENID(x)  if(kCommSucc != RobotUtils::IsValidTokenID(x))  {assert(false); return kCommFaild;}

#define CHECK_REQUESTID(x)  if(kCommSucc != RobotUtils::IsValidRequestID(x))  {assert(false); return kCommFaild;}

#define CHECK_USER(x)  if(kCommSucc != RobotUtils::IsValidUser(x))  {assert(false); return kCommFaild;}

#define CHECK_TABLE(x)  if(kCommSucc != RobotUtils::IsValidTable(x))  {assert(false); return kCommFaild;}

#define CHECK_ROOM(x)  if(kCommSucc != RobotUtils::IsValidRoom(x))  {assert(false); return kCommFaild;}

#define CHECK_ROBOT(x)  if(kCommSucc != RobotUtils::IsValidRobot(x))  {assert(false); return kCommFaild;}

#define CHECK_GAMEIP(x)  if(kCommSucc != RobotUtils::IsValidGameIP(x))  {assert(false); return kCommFaild;}

#define CHECK_GAMEPORT(x)  if(kCommSucc != RobotUtils::IsValidGamePort(x))  {assert(false); return kCommFaild;}
