#pragma once

#define		ROBOT_SETTING_FILE_NAME			_T("robot.setting")

#define		REQ_TIMEOUT_INTERVAL  4000 //请求回应等待Milliseconds

#define     ROBOT_AGENT_GROUP_ID   1 // or 888
#define		HTTP_CONNECT_TIMEOUT 5000
#define		REWORD_ONCE_AMOUNT   10000 // 一次默认申请1万两银子


#define		DEF_RESPONSE_WAIT	10000
#define		DEF_TIMER_INTERVAL	1000 // ms
#define		MIN_TIMER_INTERVAL	1000 // ms
#define		DEF_ROOMSVRPULSE_INTERVAL    10 //seconds
#define		DEF_ROOMSVRPULSE_DEADTIME    30 //seconds
#define		DEF_CLIENTPULSE_INTERVAL     10 //seconds

#define		DEF_ENTER_GAME_THREAD_NUM	 1
//RoomConfig
#define		RC_DARK_ROOM            0x00000001	// 隐名房间
#define     RC_RANDOM_ROOM          0x00000002  // 随机防作弊房间
#define     RC_CLOAKING 			0x00000008  // 隐身房
#define     RC_SOLO_ROOM			0x00000040	// Solo模式房间
#define		RC_LEAVEALONE           0x00000080  // 独自离桌模式(SOLO)模式下
#define		RC_SUPPORTMOBILE		0x00000800	// 支持移动客户端
#define		RC_VARIABLE_CHAIR		0x00001000	// 开始游戏人数可变
#define     RC_PRIVATEROOM          0x00010000  // 支持 私人包间
#define		RC_ALLBREAKNOCLEAR      0x00200000  // 所有玩家掉线时不清桌
#define     RC_WAITCHECKRESULT      0x00400000  // 游戏结果等待checksvr返回
#define		RC_TAKEDEPOSITINGAME	0x01000000	// 支持游戏里面划银
#define		RC_CHATLOG				0x04000000  // 记录房间聊天日志
#define     RC_CFG_ARENA            0x10000000  // 支持比赛-竞技场
#define     RC_CFG_YQW_ROOM         0x40000000  // 一起玩房间

#define	ROOM_PLAYER_STATUS_WALKAROUND	11	// 伺机入座（比赛中是进入房间）
#define	ROOM_PLAYER_STATUS_SEATED		12	// 已入座，（比赛中是等待下一局，不拆桌）
#define	ROOM_PLAYER_STATUS_WAITING		13	// 等待开始
#define	ROOM_PLAYER_STATUS_PLAYING		14	// 玩游戏中
#define	ROOM_PLAYER_STATUS_LOOKON		15	// 旁观
#define	ROOM_PLAYER_STATUS_BEGAN		16	// 开始游戏
using RoomID = int32_t;
using TableNO = int32_t;
using ChairNO = int32_t;
using CurUserCount = int32_t;

using          UserID = int32_t;
using         TokenID = int32_t;
using        RequestID = uint32_t;

using   TTokenSockMap = std::unordered_map<LONG, SOCKET>;

using TClientTokenMap = std::unordered_map<int, LONG>;

using       TInt32Vec = std::vector<int32_t>;

#define ERR_CONNECT_NOT_EXIST   "与服务器连接不存在"
//#define ERR_CONNECT_DISABLE     "与服务器连接已经断开"
#define ERR_CONNECT_DISABLE     "ROOM_SOCKET_ERROR"
//#define ERR_SENDREQUEST_TIMEOUT "请求处理发生超时了"
#define ERR_SENDREQUEST_TIMEOUT "ROOM_REQUEST_TIEM_OUT"
#define ERR_OPERATE_FAILED      "操作失败"
#define ERR_OPERATE_SUCESS      "请求处理OK"
#define ERR_NOT_NEED_OPERATE    "无需请求"

enum ERROR_CODE {
    CONNECT_NOT_EXIST = -1000, // "与服务器连接不存在"
    CONNECT_DISABLE, // "ROOM_SOCKET_ERROR"
    REQUEST_TIMEOUT, // "ROOM_REQUEST_TIEM_OUT"
    OPERATION_FAILED, // "操作失败"
    OPERATE_SUCESS, // "请求处理OK"
    NOT_NEED_OPERATE, // "无需请求"
};


enum EConnType :uint32_t {
    ECT_HALL,
    ECT_ROOM,
    ECT_GAME,
    ECT_MAX
};

//配置类 只读
class RobotSetting {
public:
    int32_t		userid;
    std::string	password;
    std::string nickName;
    std::string portraitUrl;
};

// 每个房间里面机器人的申请控制模式
enum EACtrlMode :uint32_t {
    EACM_Hold = 0, // 房间内机器人总是维持固定的数量
    EACM_Scale = 1,// 房间内机器人数据量总是房间内正常玩家的固定比例
    EACM_Max
};

struct RoomSetiing {
    int32_t nRoomId;
    int32_t nCtrlMode;
    int32_t nCtrlVal;
    //int32_t nCurrNum; // 当前人数
};

struct HallRoomData {
    ROOM	 room;
    uint32_t nLastGetTime;
};
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
class UThread {
public:
    UThread() {}
    UThread(std::thread&& thrd) { Initial(std::move(thrd)); }
    ~UThread() { Release(); }

public:
    void Initial(std::thread&& thrd) {
        m_thrd.swap(thrd);
        m_hThrd = (HANDLE) m_thrd.native_handle();
        m_nThrd = ::GetThreadId(m_hThrd);
    }
    void Release() {
        if (m_nThrd == 0) return;

        ::PostThreadMessage(m_nThrd, WM_QUIT, 0, 0);
        ::WaitForSingleObject(m_hThrd, WAITTIME_EXIT);
        m_thrd.detach(); //::CloseHandle(m_hThrd);
        m_nThrd = 0; m_hThrd = nullptr;
    }
    uint32_t ThreadId() { return m_nThrd; }
protected:
    std::thread     m_thrd;
    uint32_t		m_nThrd{0};
    HANDLE			m_hThrd{nullptr};
};



template <typename T>
std::shared_ptr<T> make_shared_array(std::size_t size) {
    //default_delete是STL中的默认删除器
    return std::shared_ptr<T>(new T[size], std::default_delete<T[]>());
}

using CDefSocketClientPtr = std::shared_ptr <CDefSocketClient>;

const int PluseInterval = 60; // seconds
const int DepositInterval = 15; // seconds

const int GainAmount = 200000;
const int BackAmount = 200000;

const int MaxRandomTry = 10; //避免锁的时间过长
enum class DepositType {
    kDefault,
    kGain,
    kBack,
};