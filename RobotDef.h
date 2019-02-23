#pragma once

// Ĭ�ϲ���
const int RobotAgentGroupID = 1;

const int InvalidUserID = 0;

const int InvalidGameID = 0;

const int MS_PER_SECOND = 1000;  // ms

const int RequestTimeOut = 4 * MS_PER_SECOND;  //�����Ӧ��ʱʱ��

const int PluseInterval = 60 * MS_PER_SECOND; // ����ʱ����

const int HttpTimeOut = 5 * MS_PER_SECOND;

const int MainInterval = 10 * MS_PER_SECOND; // ��ѭ��ʱ����

const int DepositInterval = 10 * MS_PER_SECOND; // ����ʱ����

const int GainAmount = 200000;

const int BackAmount = 200000;

// ҵ�����Ͷ���
using UserID = int32_t;

using GameID = int32_t;

using RoomID = int32_t;

using TableNO = int32_t;

using ChairNO = int32_t;

using TokenID = int32_t;

using RequestID = uint32_t;

using CDefSocketClientPtr = std::shared_ptr <CDefSocketClient>;

// �����˿���ģʽ
enum class EnterGameMode {
    kModeBeg = -1,
    kConstantCount = 0, // �����ڻ���������ά�̶ֹ�������
    kScaleCount = 1,// �����ڻ��������������Ƿ�����������ҵĹ̶�����
    kModeEnd = 2,
};

enum ERROR_CODE {
    kConnectionNotExist = -1000, // "����������Ӳ�����"
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
    UserID		userid;
    std::string	password;
    std::string nickname;
    std::string headurl;
};

struct RoomSetiing {
    RoomID roomid;
    EnterGameMode mode;
    int32_t count;
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

