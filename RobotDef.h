#pragma once

#define		ROBOT_SETTING_FILE_NAME			_T("robot.setting")

#define		REQ_TIMEOUT_INTERVAL  4000 //�����Ӧ�ȴ�Milliseconds

#define     ROBOT_AGENT_GROUP_ID   1 // or 888
#define		HTTP_CONNECT_TIMEOUT 5000
#define		REWORD_ONCE_AMOUNT   10000 // һ��Ĭ������1��������


#define		DEF_RESPONSE_WAIT	10000
#define		DEF_TIMER_INTERVAL	1000 // ms
#define		MIN_TIMER_INTERVAL	1000 // ms
#define		DEF_ROOMSVRPULSE_INTERVAL    10 //seconds
#define		DEF_ROOMSVRPULSE_DEADTIME    30 //seconds
#define		DEF_CLIENTPULSE_INTERVAL     10 //seconds

#define		DEF_ENTER_GAME_THREAD_NUM	 1
//RoomConfig
#define		RC_DARK_ROOM            0x00000001	// ��������
#define     RC_RANDOM_ROOM          0x00000002  // ��������׷���
#define     RC_CLOAKING 			0x00000008  // ������
#define     RC_SOLO_ROOM			0x00000040	// Soloģʽ����
#define		RC_LEAVEALONE           0x00000080  // ��������ģʽ(SOLO)ģʽ��
#define		RC_SUPPORTMOBILE		0x00000800	// ֧���ƶ��ͻ���
#define		RC_VARIABLE_CHAIR		0x00001000	// ��ʼ��Ϸ�����ɱ�
#define     RC_PRIVATEROOM          0x00010000  // ֧�� ˽�˰���
#define		RC_ALLBREAKNOCLEAR      0x00200000  // ������ҵ���ʱ������
#define     RC_WAITCHECKRESULT      0x00400000  // ��Ϸ����ȴ�checksvr����
#define		RC_TAKEDEPOSITINGAME	0x01000000	// ֧����Ϸ���滮��
#define		RC_CHATLOG				0x04000000  // ��¼����������־
#define     RC_CFG_ARENA            0x10000000  // ֧�ֱ���-������
#define     RC_CFG_YQW_ROOM         0x40000000  // һ���淿��

#define	ROOM_PLAYER_STATUS_WALKAROUND	11	// �Ż��������������ǽ��뷿�䣩
#define	ROOM_PLAYER_STATUS_SEATED		12	// �����������������ǵȴ���һ�֣���������
#define	ROOM_PLAYER_STATUS_WAITING		13	// �ȴ���ʼ
#define	ROOM_PLAYER_STATUS_PLAYING		14	// ����Ϸ��
#define	ROOM_PLAYER_STATUS_LOOKON		15	// �Թ�
#define	ROOM_PLAYER_STATUS_BEGAN		16	// ��ʼ��Ϸ

typedef int32_t			TUserId;
typedef int32_t			TTokenId;
typedef uint32_t		TReqstId;

typedef std::unordered_map<LONG, SOCKET>	TTokenSockMap;

typedef std::unordered_map<int, LONG>		TClientTokenMap;

typedef std::vector<int32_t>				TInt32Vec;



typedef std::tuple<bool, std::string>		TTueRet;
#define TUPLE_NUM(_t_) std::tuple_size<decltype(_t_)>::value
#define TUPLE_ELE(_t_,_index_) std::get<_index_>(_t_)
#define TUPLE_ELE_C(_t_,_index_) std::get<_index_>(_t_).c_str()

#define ERR_CONNECT_NOT_EXIST   "����������Ӳ�����"
//#define ERR_CONNECT_DISABLE     "������������Ѿ��Ͽ�"
#define ERR_CONNECT_DISABLE     "ROOM_SOCKET_ERROR"
//#define ERR_SENDREQUEST_TIMEOUT "������������ʱ��"
#define ERR_SENDREQUEST_TIMEOUT "ROOM_REQUEST_TIEM_OUT"
#define ERR_OPERATE_FAILED      "����ʧ��"
#define ERR_OPERATE_SUCESS      "������OK"
#define ERR_NOT_NEED_OPERATE    "��������"



enum EConnType :uint32_t {
    ECT_HALL,
    ECT_ROOM,
    ECT_GAME,
    ECT_MAX
};
struct THREAD_INFO {
    void* lpVoid;
    int	  nIndex;
};
typedef THREAD_INFO *LPTHREAD_INFO;

class stRobotUnit {
public:
    int32_t		account;
    std::string	password;
    std::string nickName;
    std::string portraitUrl;
    bool		logoned;
};
// ÿ��������������˵��������ģʽ
enum EACtrlMode :uint32_t {
    EACM_Hold = 0, // �����ڻ���������ά�̶ֹ�������
    EACM_Scale = 1,// �����ڻ��������������Ƿ�����������ҵĹ̶�����
    EACM_Max
};

struct stActiveCtrl {
    int32_t nRoomId;
    int32_t nCtrlMode;
    int32_t nCtrlVal;
    int32_t nCurrNum; // ��ǰ����
};

struct stRoomData {
    ROOM	 room;
    uint32_t nLastGetTime;
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