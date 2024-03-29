#pragma once
#include "robot_define.h"
#include "user.h"
#include "table.h"
#include "base_room.h"

//无状态类 不应该有任何成员变量 保持线程安全
class RobotUtils {
public:
    // 某个特定的connection发送协议
    static int SendRequestWithLock(const CDefSocketClientPtr& connection, const RequestID& requestid, const google::protobuf::Message &val, REQUEST& response, const bool& need_echo = true);

    // 发送HTTP POST协议
    static CString ExecHttpRequestPost(const CString& url, const CString& params);

    // 产生随机数
    static int GenRandInRange(const int64_t& min_value, const int64_t& max_value, int64_t& random_result);

    // 获得配置游戏服务器地址
    static std::string GetGameIP();

    // 线程阻塞ms 一般用于测试和限流
    static int Sleep(const uint32_t& milli_seconds);

    // 参数检查
    static int IsValidGameID(const GameID& game_id);

    static int IsValidUserID(const UserID& userid);

    static int IsValidRoomID(const RoomID& roomid);

    static int IsValidTableNO(const TableNO& tableno);

    static int IsValidChairNO(const ChairNO& chairno);

    static int IsValidTokenID(const TokenID& tokenid);

    static int IsValidRequestID(const RequestID& requestid);

    static int IsValidUser(const UserPtr& user);

    static int IsValidTable(const TablePtr& table);

    static int IsValidRoom(const RoomPtr& room);

    static int IsValidGameIP(const std::string& game_ip);

    static int IsValidGamePort(const int32_t& game_port);

    static int IsValidConnection(const CDefSocketClientPtr& connection);

    static int IsValidThreadID(const ThreadID& thead_id);

    static int IsNegativeDepositAmount(const int64_t& deposit_amount);

    // 控制方法对特定线程可见
    static int IsCurrentThread(YQThread& thread);

    // 控制方法对特定线程不可见
    static int NotThisThread(YQThread& thread);

    // 只允许合法线程
    static int IsAllowedThreadID();

    // 打印调用栈
    static int TraceStack();

    static std::string ErrorCodeInfo(int code);

    static std::string RequestStr(const RequestID& requestid);

    static std::string UserTypeStr(const int& type);

    static std::string TableStatusStr(const int& status);

    static std::string ChairStatusStr(const int& status);
};

#define SLEEP_FOR(x) RobotUtils::Sleep(x);

#define CHECK_GAMEID(x) if(kCommSucc != RobotUtils::IsValidGameID(x))  {ASSERT_FALSE_RETURN}

#define CHECK_USERID(x)  if(kCommSucc != RobotUtils::IsValidUserID(x))  {ASSERT_FALSE_RETURN}

#define CHECK_ROOMID(x)  if(kCommSucc != RobotUtils::IsValidRoomID(x)) { ASSERT_FALSE_RETURN}

#define CHECK_TABLENO(x)  if(kCommSucc != RobotUtils::IsValidTableNO(x))  {ASSERT_FALSE_RETURN}

#define CHECK_CHAIRNO(x)  if(kCommSucc != RobotUtils::IsValidChairNO(x))  {ASSERT_FALSE_RETURN}

#define CHECK_TOKENID(x)  if(kCommSucc != RobotUtils::IsValidTokenID(x))  {ASSERT_FALSE_RETURN}

#define CHECK_REQUESTID(x)  if(kCommSucc != RobotUtils::IsValidRequestID(x))  {ASSERT_FALSE_RETURN}

#define CHECK_USER(x)  if(kCommSucc != RobotUtils::IsValidUser(x))  {ASSERT_FALSE_RETURN}

#define CHECK_CONNECTION(x)  if(kCommSucc != RobotUtils::IsValidConnection(x))  {ASSERT_FALSE_RETURN}

#define CHECK_TABLE(x)  if(kCommSucc != RobotUtils::IsValidTable(x))  {ASSERT_FALSE_RETURN}

#define CHECK_ROOM(x)  if(kCommSucc != RobotUtils::IsValidRoom(x))  {ASSERT_FALSE_RETURN}

#define CHECK_GAMEIP(x)  if(kCommSucc != RobotUtils::IsValidGameIP(x))  {ASSERT_FALSE_RETURN}

#define CHECK_GAMEPORT(x)  if(kCommSucc != RobotUtils::IsValidGamePort(x))  {ASSERT_FALSE_RETURN}

#define CHECK_THREADID(x)  if(kCommSucc != RobotUtils::IsValidThreadID(x))  {ASSERT_FALSE_RETURN}

#define CHECK_THREAD(x)  if(kCommSucc != RobotUtils::IsCurrentThread(x))  {ASSERT_FALSE_RETURN}

#define CHECK_NOT_THREAD(x)  if(kCommSucc != RobotUtils::NotThisThread(x))  {ASSERT_FALSE_RETURN}

#define CHECK_MAIN_OR_LAUNCH_THREAD()  if(kCommSucc != RobotUtils::IsAllowedThreadID())  {ASSERT_FALSE_RETURN}

#define CHECK_DEPOSIT(x)  if(kCommSucc != RobotUtils::IsNegativeDepositAmount(x))  {ASSERT_FALSE_RETURN}

#define ERR_STR(x) RobotUtils::ErrorCodeInfo(x).c_str()

#define REQ_STR(x) RobotUtils::RequestStr(x).c_str()

#define USER_TYPE_STR(x) RobotUtils::UserTypeStr(x).c_str()

#define TABLE_STATUS_STR(x) RobotUtils::TableStatusStr(x).c_str()

#define CHAIR_STATUS_STR(x) RobotUtils::ChairStatusStr(x).c_str()

#define LOG_INFO_FUNC(x) LOG_INFO(" %s [%s]", x, __FUNCTION__);

#define DEBUG_FUNC LOG_DEBUG("[%s]",__FUNCTION__);

#define LOG_ROUTE(x, roomid, tableno ,userid) LOG_DEBUG(" [%s] roomid [%d] tableno [%d] userid [%d] [%s]", "DISPATCH", roomid, tableno, userid, x);

#define DEBUG_USER(userid) LOG_DEBUG(" [%s] userid [%d]", __FUNCTION__, userid);

// 打印堆栈
#define TRACE_STACK RobotUtils::TraceStack();

// 严格模式 直接断言
#ifdef STRICT_ASSERT

#define  ASSERT_FALSE {\
                        TRACE_STACK \
                        assert(false); \
                                       }

#define  ASSERT_FALSE_RETURN { \
                                TRACE_STACK \
                                ASSERT_FALSE \
                                return kCommFaild; \
                                                     }
#else

#define  ASSERT_FALSE TRACE_STACK

#define  ASSERT_FALSE_RETURN {\
                                TRACE_STACK \
                                return kCommFaild; \
                                                       }
#endif


