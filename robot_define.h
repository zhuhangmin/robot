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

const int MS_PER_SECOND = 1000;  // ms

const int RequestTimeOut = 4 * MS_PER_SECOND;  //�����Ӧ��ʱʱ�� 4

const int PulseInterval = 60 * MS_PER_SECOND; // ����ʱ���� 60

const int RobotTimerInterval = 1 * MS_PER_SECOND; // ��ѯ1��, ��� 60

const int HttpTimeOut = 5 * MS_PER_SECOND; // 5

const int MainInterval = 10 * MS_PER_SECOND; // ��ѭ��ʱ���� 10

const int DepositInterval = 10 * MS_PER_SECOND; // ����ʱ����  10

const int GainAmount = 200000;

const int BackAmount = 200000;

const int InitGainFlag = 1;

const int MaxPluseTimeOutCount = 3;

// ҵ�����Ͷ���
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

enum RobotErrorCode {
    kConnectionNotExist = -6666, // "����������Ӳ�����"
    kConnectionDisable, // "ROOM_SOCKET_ERROR"
    kConnectionTimeOut, // "ROOM_REQUEST_TIEM_OUT"
    kOperationFailed, // "����ʧ��"
    kOperationSuccess, // "������OK"
    kOperationNoNeed, // "��������"
};

//#define ERR_CONNECT_NOT_EXIST   "����������Ӳ�����"
//#define ERR_CONNECT_DISABLE     "ROOM_SOCKET_ERROR"
//#define ERR_SENDREQUEST_TIMEOUT "ROOM_REQUEST_TIEM_OUT"
//#define ERR_OPERATE_FAILED      "����ʧ��"
//#define ERR_OPERATE_SUCESS      "������OK"
//#define ERR_NOT_NEED_OPERATE    "��������"

enum class DepositType {
    kDefault,
    kGain, //����
    kBack, //����
};

enum class HallLogonStatusType {
    kNotLogon,
    kLogon,
};

//������ ֻ��
class RobotSetting {
public:
    UserID	userid;
    std::string	password;
    std::string nickname;
    std::string headurl;
};

struct RoomSetiing {
    RoomID roomid;
    int count;
};

struct HallRoomData {
    ROOM	 room;
};

// ����ģ����
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

// �̶߳���
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

using CDefSocketClientPtr = std::shared_ptr <CDefSocketClient>;

using RoomSettingMap = std::unordered_map<RoomID, RoomSetiing>;

using RobotSettingMap = std::unordered_map<UserID, RobotSetting>;

using HallLogonMap = std::unordered_map<UserID, HallLogonStatusType>;

using HallRoomDataMap = std::unordered_map<RoomID, HallRoomData>;

using DepositMap = std::unordered_map<UserID, DepositType>;

using RoomNeedCountMap = std::unordered_map<RoomID, NeedCount>;

using SendMsgCountMap = std::unordered_map<RequestID, uint64_t>;

using RecvMsgCountMap = std::unordered_map<RequestID, uint64_t>;


