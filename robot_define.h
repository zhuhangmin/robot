#pragma once

#define SERVICE_NAME _T("robottool")

#define DISPLAY_NAME _T("robottoolxxx")

//#define STRICT_ASSERT // 开启严格调试模式, 错误时打印调用栈 并assert 

#define CURRENT_DELAY // 限流延时标签

const std::string LocalIPStr = std::string("127.0.0.1");

// Invalid Param
const int InvalidPort = 0;

const int RobotAgentGroupID = 1;

const int InvalidUserID = 0;

const int InvalidGameID = 0;

const int InvalidRoomID = 0;

const int InvalidTableNO = 0;

const int InvalidChairNO = 0;

const int InvalidTokenID = 0;

const int InvalidRequestID = 0;

const int InvalidCount = -1;

const int InvalidThreadID = 0;

const int InvallidPort = 0;

// Time Interval
const int MS_PER_SECOND = 1000;  // ms

const int TimerInterval = 1 * MS_PER_SECOND; // 定时器时间间隔 1s

const int GamePulseInterval = 30; // 心跳时间间隔 30s

const int HallPulseInterval = 30; // 心跳时间间隔 30s

const int RobotPulseInterval = 60; // 心跳时间间隔 60s

const int RequestTimeOut = 4 * MS_PER_SECOND;  //请求回应超时时间 4s

const int HttpTimeOut = 5 * MS_PER_SECOND; // HTTP 超时 5s

const int MainInterval = 10 * MS_PER_SECOND; // 主循环时间间隔 10s

const int DepositInterval = 1 * MS_PER_SECOND; // 补银时间间隔  1s

// 补银还银
const int64_t GainAmount = 200000;

const int64_t BackAmount = 200000;

const int64_t MaxAmount = 200000000; // 后台同事确认正式最大补银。后台可配置

const float DepoistFactor = 0.7f; // rummy 补银业务参数

// 心跳超时计数
const int MaxPluseTimeOutCount = 5;

// 限流延时参数
const int CurrentDealy = 50; // ms

// 运行统计开关
const int ProcessInfoClose = 0;

const int ProcessInfoOpen = 1;

// 业务类型定义
using UserID = int32_t;

using GameID = int32_t;

using RoomID = int32_t;

using TableNO = int32_t;

using ChairNO = int32_t;

using TokenID = int32_t;

using RequestID = uint32_t;

using ThreadID = uint32_t;

using NeedCount = uint32_t;

using TimeStamp = time_t;

using EventKey = uint32_t;

enum class EventType {
    kBeg = 0,
    kSend,
    kRecv,
    kErr,
    kEnd,
};

enum RobotErrorCode {
    kCreateHallConnFailed = 10086,
    kCreateGameConnFailed,
    kConnectionTimeOut,
    kOperationFailed,
    kRespIDZero,
};

enum RobotExceptionCode {
    kExceptionUserNotOnChair = 20086,
    kExceptionNoRobotDeposit,
    kExceptionNoMoreRobot,
    kExceptionGameDataNotInited,
    kExceptionRobotNotInGame,
    kExceptionNotRobotType,
    kExceptionRobotInGame,
};

enum class DepositType {
    kDefault,
    kGain, //补银
    kBack, //还银
};

enum class HallLogonStatusType {
    kNotLogon,
    kLogon,
};

//配置类 只读
class RobotSetting {
public:
    UserID	userid;
    std::string	password;
    std::string nickname;
    std::string headurl;
};

struct RoomSetiing {
    RoomID roomid;
    int wait_time;
    int count_per_table;
};

struct HallRoomData {
    ROOM	 room;
};

struct NeedUser {
    UserID userid;
    TableNO tableno;
    RoomID roomid;
};

struct DepositData {
    DepositType type;
    int64_t amount;
};

typedef struct _tagQUERY_USER_GAMEINFO {
    int nUserID;
    int nGameID;
    DWORD dwQueryFlags;
    UWL_ADDR stIPAddr;
    char szHardID[MAX_HARDID_LEN];
    INT64 nGiftDeposit;
    int nReserved[4];
}QUERY_USER_GAMEINFO, *LPQUERY_USER_GAMEINFO;

typedef struct _tagUSER_GAMEINFO_MB {
    int nUserID;                // 用户ID
    int nGameID;                // 游戏ID
    INT64  nDeposit;                // 银子
    int nPlayerLevel;              // 级别
    int nScore;                  // 积分
    int nExperience;              // 经验(分钟，已废弃)
    int nBreakOff;                // 断线
    int nWin;                  // 赢
    int nLoss;                  // 输
    int nStandOff;                // 和
    int nBout;                  // 回合
    int nTimeCost;                // 花时(秒)
    int nSalaryTime;
    INT64 nSalaryDeposit;
    INT64 nTotalSalary;
    DWORD dwFlags;                              // 一些状态标记
    int nReserved[7];
}USER_GAMEINFO_MB, *LPUSER_GAMEINFO_MB;//注意这个结构跟pc端的那个不同

using CDefSocketClientPtr = std::shared_ptr <CDefSocketClient>;

using RoomSettingMap = std::unordered_map<RoomID, RoomSetiing>;

using RobotSettingMap = std::unordered_map<UserID, RobotSetting>;

using HallLogonMap = std::unordered_map<UserID, HallLogonStatusType>;

using HallRoomDataMap = std::unordered_map<RoomID, HallRoomData>;

using UserDepositMap = std::unordered_map<UserID, int64_t>;

using DepositMap = std::unordered_map<UserID, DepositData>;

using RoomNeedCountMap = std::unordered_map<RoomID, NeedCount>;

using RoomNeedUserMap = std::unordered_map<RoomID, NeedUser>;

using SendMsgCountMap = std::unordered_map<RequestID, uint64_t>;

using RecvNtfCountMap = std::unordered_map<RequestID, uint64_t>;

using ErrorCountMap = std::unordered_map<uint32_t, uint64_t>;

using RobotUserIDMap = std::unordered_map<UserID, UserID>;

using RobotDepositMap = std::unordered_map<UserID, int64_t>;

class User;
using UserMap = std::hash_map<UserID, std::shared_ptr<User>>;

using UserFilterMap = std::hash_map<UserID, UserID>;

// 单例模板类
template< typename T >
class ISingletion {
public:
    ISingletion(const ISingletion&) = delete;
    ISingletion& operator = (const ISingletion&) = delete;
    ISingletion(ISingletion&& t) = delete;
    ISingletion& operator = (ISingletion&& t) = delete;
    virtual ~ISingletion() {}

    static T& Instance() {
        static T s_xInstance;
        return s_xInstance;
    }

protected:
    ISingletion() {}
};

#define SINGLETION_CONSTRUCTOR(_class_name_) \
friend ISingletion < _class_name_ >; \
_class_name_(){}; \
virtual ~_class_name_(){}; \

// 线程对象
class YQThread {
public:
    YQThread() {}
    explicit YQThread(std::thread&& thrd) { Initial(std::move(thrd)); }
    ~YQThread() { Release(); }

public:
    void Initial(std::thread&& thrd) {
        m_thrd.swap(thrd);
        m_hThrd = static_cast<HANDLE>(m_thrd.native_handle());
        m_nThrd = GetThreadId(m_hThrd);
    }
    void Release() {
        if (m_nThrd == 0) return;

        PostThreadMessage(m_nThrd, WM_QUIT, 0, 0);
        WaitForSingleObject(m_hThrd, WAITTIME_EXIT);
        m_thrd.detach(); //::CloseHandle(m_hThrd);
        m_nThrd = 0; m_hThrd = nullptr;
    }
    ThreadID GetThreadID() const { return m_nThrd; }
protected:
    std::thread     m_thrd;
    ThreadID		m_nThrd{0};
    HANDLE			m_hThrd{nullptr};
};




