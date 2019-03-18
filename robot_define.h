#pragma once

#define SERVICE_NAME _T("robot_tool_xxx")

#define DISPLAY_NAME _T("robot_tool_xxx")

const std::string LocalIPStr = std::string("127.0.0.1");

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

const int InvalidThreadID = -1;

const int MS_PER_SECOND = 1000;  // ms

const int TimerInterval = 1 * MS_PER_SECOND; // 定时器时间间隔 1

const int GamePulseInterval = 60 * MS_PER_SECOND; // 心跳时间间隔 60

const int HallPulseInterval = 60 * MS_PER_SECOND; // 心跳时间间隔 60

const int RobotPulseInterval = 60 * MS_PER_SECOND; // 心跳时间间隔 60

const int GetAllRoomDataInterval = 10 * 60 * MS_PER_SECOND; // 10 min  

const int RequestTimeOut = 4 * MS_PER_SECOND;  //请求回应超时时间 4

// 游戏服务器兜底状态同步, 请勿减少时间间隔会触发后端服务器大量锁
const int GameSyncInterval = 10 * 60 * MS_PER_SECOND; // 10 min  

const int HttpTimeOut = 5 * MS_PER_SECOND; // HTTP 超时 5

const int MainInterval = 10 * MS_PER_SECOND; // 主循环时间间隔 10

const int DepositInterval = 10 * MS_PER_SECOND; // 补银时间间隔  10

const int HotUpdateInterval = 10 * MS_PER_SECOND; // 配置热更新  10

const int64_t GainAmount = 200000;

const int64_t BackAmount = 200000;

const int InitGainFlag = 1;

const int MaxPluseTimeOutCount = 5;

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

using UserGameInfoMap = std::unordered_map<UserID, USER_GAMEINFO_MB>;

using DepositMap = std::unordered_map<UserID, DepositData>;

using RoomNeedCountMap = std::unordered_map<RoomID, NeedCount>;

using RoomNeedUserMap = std::unordered_map<RoomID, NeedUser>;

using SendMsgCountMap = std::unordered_map<RequestID, uint64_t>;

using RecvNtfCountMap = std::unordered_map<RequestID, uint64_t>;

using ErrorCountMap = std::unordered_map<uint32_t, uint64_t>;

using RobotUserIDMap = std::unordered_map<UserID, UserID>;

using RobotDepositMap = std::unordered_map<UserID, int64_t>;


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




