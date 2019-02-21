#pragma once

#define PORT_OF_CHECKSVR		30623		// port
#define CONNECT_CHECKSVR_WAIT	10			// waittime(seconds)
#define	CONNECTS_TO_CHECKSVR	1			// connects
#define	CLIENT_INITIAL_RECVS	10			// recvs


// ȫ����Ϣ����10000000����
// 10010000 <= �������� < 10020000
// 10020000 <= ��Ϸģ�� < 10025000
// 10025000 <= ��Ϸ���� < 10030000
// G��ʾ��Ϸ������أ�H��ʾ����������أ�R��ʾ����N��ʾ֪ͨ
enum HallRequestEnum {
    HR_REQUEST_BASE = 10010000,
    HR_ENTER_GAME = HR_REQUEST_BASE + 1,

    HN_ENTER_GAME_OK = HR_REQUEST_BASE + 1001
};

enum GameRequestEnum {
    GR_REQUEST_BASE = 10020000,

    GR_ENTER_NORMAL_GAME = GR_REQUEST_BASE + 1,			// ����ͨ��
    GR_ENTER_PRIVATE_GAME = GR_REQUEST_BASE + 2,		// ��˽�˷�
    GR_ENTER_MATCH_GAME = GR_REQUEST_BASE + 3,			// ��������
    GR_LEAVE_GAME = GR_REQUEST_BASE + 21,				// �뿪��Ϸ
    GR_GIVE_UP = GR_REQUEST_BASE + 31,					// ����
    //GR_START_GAME = GR_REQUEST_BASE + 32,				// ��ʼ��Ϸ�������ڲ���ʱ��������
    GR_GAME_PLUSE = GR_REQUEST_BASE + 33,				// ����
    GR_SWITCH_TABLE = GR_REQUEST_BASE + 34,				// ��������
    GR_TABLE_CHAT = GR_REQUEST_BASE + 35,				// ͬ������

    GN_USER_STATUS_CHANGE = GR_REQUEST_BASE + 1001,		// �û�״̬�仯֪ͨ
    GN_TABLE_CHAT = GR_REQUEST_BASE + 1002,				// ������Ϣ֪ͨ
    GN_COUNTDOWN_START = GR_REQUEST_BASE + 1003,		// ����ʱ��ʼ֪ͨ
    GN_COUNTDOWN_STOP = GR_REQUEST_BASE + 1004,			// ����ʱֹ֪ͣͨ
    GN_GAME_START = GR_REQUEST_BASE + 1005,				// ��Ϸ��ʼ֪ͨ
    GN_GAME_RESULT = GR_REQUEST_BASE + 1006,			// ��Ϸ���֪ͨ
    GN_PLAYER_GIVEUP = GR_REQUEST_BASE + 1007,			// �������֪ͨ

    //========= �����������Ϣ ==========
    GR_VALID_ROBOTSVR = GR_REQUEST_BASE + 4001,			// �����˷�����֤��Ϣ
    //GR_GET_GAMEUSERS = GR_REQUEST_BASE + 4002,			// �����˷����ȡ��Ϸ�����������Ϣ(room��table��user)

    //RS��ʾRobotSvr
    GN_RS_PLAER_ENTERGAME = GR_REQUEST_BASE + 4501,		// ��ҽ�����Ϸ	BindPlayer
    GN_RS_LOOKER_ENTERGAME = GR_REQUEST_BASE + 4502,	// �Թ��߽�����Ϸ	BindLooker
    GN_RS_LOOER2PLAYER = GR_REQUEST_BASE + 4503,		// �Թ�ת���	BindPlayer
    GN_RS_PLAYER2LOOKER = GR_REQUEST_BASE + 4504,		// ���ת�Թ�	UnbindPlayer
    GN_RS_GAME_START = GR_REQUEST_BASE + 4505,			// ��ʼ��Ϸ	StartGame(Я�����Ӻţ�)
    GN_RS_USER_REFRESH_RESULT = GR_REQUEST_BASE + 4506,	// �û����˽���	RefreshGameResult(int userid)
    GN_RS_REFRESH_RESULT = GR_REQUEST_BASE + 4507,		// ��������	RefreshGameResult
    GN_RS_USER_LEAVEGAME = GR_REQUEST_BASE + 4508,		// �û��뿪��Ϸ	UnbindUser
    GN_RS_SWITCH_TABLE = GR_REQUEST_BASE + 4509,		// �û�����	UnbindUser+BindPlayer

};

// 0~-100Ϊͨ�ô�����
// -1000~-2000Ϊģ�������
// -2000~-5000Ϊ��Ϸ�Զ��������
enum ErrorCode {
    kCommSucc = 0,
    kCommFaild = -1,
    kInvalidParam = -2,
    kInternalErr = -3,

    kInvalidUser = -1000,			// �û����Ϸ���token��entergameʱ�����token��һ�£�
    kInvalidRoomID = -1001,			// ���䲻����
    kAllocTableFaild = -1002,		// ��������ʧ��
    kTargetUserNotFound = -1003,	// Ŀ���û�������
    kTargetTableNotFound = -1004,	// Ŀ�����Ӳ�����
    kDepositInvalid = -1005			// ������������������TODO��ȷ��վ��������ʧ�ܣ��Ƿ���ϲ�Ʒ���󣿣�
};

enum EnterGameFlag {
    kEnterDefault = 0x00000000,
    kLookerEnterGame = 0x00000001
};

enum EnterGameRespFlag {
    // ����λ������������
    kResp_EnterNormalGame = 0x00000001,
    kResp_EnterPrivateGame = 0x00000002,
    kResp_EnterMatchGame = 0x00000004,

    //
    kResp_PlayerEnterGame = 0x01000000,
    kResp_LookerEnterGame = 0x02000000,

    // ��һλ����Ƿ��Ƕ�������
    kResp_ContinueGame = 0x10000000
};

enum {
    kMinChairCountPerTable = 1,
    kMaxChairCountPerTable = 8,
    kMinTableCountPerRoom = 1,
    kMaxTableCountPerRoom = 2000
};

// ģ��ʹ�ú���λ������������Ϸ��ʹ��
enum TableStatus {
    kTablePlaying = 0x00000001,		// ��Ϸ������
    kTableWaiting = 0x00000002,		// �ȴ���Ϸ��ʼ
};

enum ChairStatus {
    kChairPlaying = 0x00000001,		// ��Ϸ������
    kChairWaiting = 0x00000002,		// �ȴ���Ϸ��ʼ
};

// ���״̬�Ƶ���
// �����Ϣ�����Ӻ�Ϊ0��˵�����Թۣ�
// �����Ϣ��tokenΪ0��˵��������ߣ�
// �����Ӻ���鿴����״̬������waiting -> ���waiting
// ����playing && ����playing -> ���playing
// ����playing && ����waiting -> �ȴ��¾���Ϸ��ʼ��ԭ������ң�
enum UserStatus {
    kUserWaiting = 0x00000001,		// �ȴ���Ϸ��ʼ
    kUserPlaying = 0x00000002,		// ��Ϸ��
    kUserLooking = 0x00000004,		// �Թ�
    kUserLeaved = 0x00000008,		// �뿪��Ϸ
    kUserOffline = 0x10000000		// ����
};

enum UserType {
    kUserNormal = 0x00000000,		// ��ͨ�û�
    kUserAdmin = 0x00001000,		// ����Ա
    kUserSuperAdmin = 0x00002000,	// ��������Ա
    kUserRobot = 0x40000000			// ������
};

enum Enable {
    kEnableOff = 0,
    kEnableOn = 1
};