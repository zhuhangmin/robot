#pragma once

#define PORT_OF_CHECKSVR		30623		// port
#define CONNECT_CHECKSVR_WAIT	10			// waittime(seconds)
#define	CONNECTS_TO_CHECKSVR	1			// connects
#define	CLIENT_INITIAL_RECVS	10			// recvs


// 全新消息号在10000000以上
// 10010000 <= 大厅服务 < 10020000
// 10020000 <= 游戏模板 < 10025000
// 10025000 <= 游戏服务 < 10030000
// G表示游戏服务相关，H表示大厅服务相关，R表示请求，N表示通知
enum HallRequestEnum {
    HR_REQUEST_BASE = 10010000,
    HR_ENTER_GAME = HR_REQUEST_BASE + 1,

    HN_ENTER_GAME_OK = HR_REQUEST_BASE + 1001
};

enum GameRequestEnum {
    GR_REQUEST_BASE = 10020000,

    GR_ENTER_NORMAL_GAME = GR_REQUEST_BASE + 1,			// 进普通房
    GR_ENTER_PRIVATE_GAME = GR_REQUEST_BASE + 2,		// 进私人房
    GR_ENTER_MATCH_GAME = GR_REQUEST_BASE + 3,			// 进比赛房
    GR_LEAVE_GAME = GR_REQUEST_BASE + 21,				// 离开游戏
    GR_GIVE_UP = GR_REQUEST_BASE + 31,					// 弃牌
    //GR_START_GAME = GR_REQUEST_BASE + 32,				// 开始游戏（服务内部定时器触发）
    GR_GAME_PLUSE = GR_REQUEST_BASE + 33,				// 心跳
    GR_SWITCH_TABLE = GR_REQUEST_BASE + 34,				// 换桌请求
    GR_TABLE_CHAT = GR_REQUEST_BASE + 35,				// 同桌聊天

    GN_USER_STATUS_CHANGE = GR_REQUEST_BASE + 1001,		// 用户状态变化通知
    GN_TABLE_CHAT = GR_REQUEST_BASE + 1002,				// 聊天消息通知
    GN_COUNTDOWN_START = GR_REQUEST_BASE + 1003,		// 倒计时开始通知
    GN_COUNTDOWN_STOP = GR_REQUEST_BASE + 1004,			// 倒计时停止通知
    GN_GAME_START = GR_REQUEST_BASE + 1005,				// 游戏开始通知
    GN_GAME_RESULT = GR_REQUEST_BASE + 1006,			// 游戏结果通知
    GN_PLAYER_GIVEUP = GR_REQUEST_BASE + 1007,			// 玩家弃牌通知

    //========= 机器服相关消息 ==========
    GR_VALID_ROBOTSVR = GR_REQUEST_BASE + 4001,			// 机器人服务验证消息
    //GR_GET_GAMEUSERS = GR_REQUEST_BASE + 4002,			// 机器人服务获取游戏内所有玩家信息(room、table、user)

    //RS表示RobotSvr
    GN_RS_PLAER_ENTERGAME = GR_REQUEST_BASE + 4501,		// 玩家进入游戏	BindPlayer
    GN_RS_LOOKER_ENTERGAME = GR_REQUEST_BASE + 4502,	// 旁观者进入游戏	BindLooker
    GN_RS_LOOER2PLAYER = GR_REQUEST_BASE + 4503,		// 旁观转玩家	BindPlayer
    GN_RS_PLAYER2LOOKER = GR_REQUEST_BASE + 4504,		// 玩家转旁观	UnbindPlayer
    GN_RS_GAME_START = GR_REQUEST_BASE + 4505,			// 开始游戏	StartGame(携带椅子号？)
    GN_RS_USER_REFRESH_RESULT = GR_REQUEST_BASE + 4506,	// 用户单人结算	RefreshGameResult(int userid)
    GN_RS_REFRESH_RESULT = GR_REQUEST_BASE + 4507,		// 整桌结算	RefreshGameResult
    GN_RS_USER_LEAVEGAME = GR_REQUEST_BASE + 4508,		// 用户离开游戏	UnbindUser
    GN_RS_SWITCH_TABLE = GR_REQUEST_BASE + 4509,		// 用户换桌	UnbindUser+BindPlayer

};

// 0~-100为通用错误码
// -1000~-2000为模板错误码
// -2000~-5000为游戏自定义错误码
enum ErrorCode {
    kCommSucc = 0,
    kCommFaild = -1,
    kInvalidParam = -2,
    kInternalErr = -3,

    kInvalidUser = -1000,			// 用户不合法（token和entergame时保存的token不一致）
    kInvalidRoomID = -1001,			// 房间不存在
    kAllocTableFaild = -1002,		// 分配桌子失败
    kTargetUserNotFound = -1003,	// 目标用户不存在
    kTargetTableNotFound = -1004,	// 目标桌子不存在
    kDepositInvalid = -1005			// 银子数不符合条件（TODO：确认站起再坐下失败，是否符合产品需求？）
};

enum EnterGameFlag {
    kEnterDefault = 0x00000000,
    kLookerEnterGame = 0x00000001
};

enum EnterGameRespFlag {
    // 后两位留给房间类型
    kResp_EnterNormalGame = 0x00000001,
    kResp_EnterPrivateGame = 0x00000002,
    kResp_EnterMatchGame = 0x00000004,

    //
    kResp_PlayerEnterGame = 0x01000000,
    kResp_LookerEnterGame = 0x02000000,

    // 第一位标记是否是断线续玩
    kResp_ContinueGame = 0x10000000
};

enum {
    kMinChairCountPerTable = 1,
    kMaxChairCountPerTable = 8,
    kMinTableCountPerRoom = 1,
    kMaxTableCountPerRoom = 2000
};

// 模板使用后两位，其他留给游戏方使用
enum TableStatus {
    kTablePlaying = 0x00000001,		// 游戏进行中
    kTableWaiting = 0x00000002,		// 等待游戏开始
};

enum ChairStatus {
    kChairPlaying = 0x00000001,		// 游戏进行中
    kChairWaiting = 0x00000002,		// 等待游戏开始
};

// 玩家状态推导：
// 玩家信息中椅子号为0则说明在旁观；
// 玩家信息中token为0则说明玩家离线；
// 有椅子号则查看桌子状态，桌子waiting -> 玩家waiting
// 桌子playing && 椅子playing -> 玩家playing
// 桌子playing && 椅子waiting -> 等待下局游戏开始（原空闲玩家）
enum UserStatus {
    kUserWaiting = 0x00000001,		// 等待游戏开始
    kUserPlaying = 0x00000002,		// 游戏中
    kUserLooking = 0x00000004,		// 旁观
    kUserLeaved = 0x00000008,		// 离开游戏
    kUserOffline = 0x10000000		// 断线
};

enum UserType {
    kUserNormal = 0x00000000,		// 普通用户
    kUserAdmin = 0x00001000,		// 管理员
    kUserSuperAdmin = 0x00002000,	// 超级管理员
    kUserRobot = 0x40000000			// 机器人
};

enum Enable {
    kEnableOff = 0,
    kEnableOn = 1
};