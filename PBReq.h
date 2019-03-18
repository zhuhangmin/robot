
#define		PB_REQ_BASE          100000         //此数据值必须固定,应对版本升级

#define     PB_GET_FRIENDS_LIST					(PB_REQ_BASE +    1)  // 获取好友列表
#define     PB_GET_FRIENDS_ONLINESTATUS			(PB_REQ_BASE +    2)  // 查询好友状态
#define     PB_GET_FRIENDS_DEPOSIT				(PB_REQ_BASE +    3)  // 查询好友银子
#define     PB_INVITE_FRIEND					(PB_REQ_BASE +    4)  // 邀请好友
#define     PB_SEARCH_PLAYER_INGAME				(PB_REQ_BASE +    5)  // 查询用户当前所在位置

#define     PB_INVITE_FRIEND_OK					(PB_REQ_BASE +   10)  // 邀请好友回应
#define     PB_INVITED_BY_FRIEND				(PB_REQ_BASE +   11)  // 邀请好友通知
#define		PB_FRIENDS_ONLINESTATUS				(PB_REQ_BASE +   12)  // 好友状态通知
#define     PB_INVITE_GIFT						(PB_REQ_BASE +   13)  // 邀请有礼通知


#define     PB_GET_AREA_ROOMS					(PB_REQ_BASE +   20)  // 获取区域和房间列表

#define		PB_NOTIFY_TO_CLIENT					(PB_REQ_BASE +   200) // 发送给客户端的通知
#define		PB_NOTIFY_ACK						(PB_REQ_BASE +   201) // 客户端已收到通知确认
#define		PB_GET_NOTIFYS						(PB_REQ_BASE +   202) // 获取未收到的通知

#define		PB_UA_LOAD_GAME						(PB_REQ_BASE +   300) // 用户行为上报-加载游戏界面

//////////////////////////////////////////////////////////////////////////
// PB_REQ_BASE + 1000 ~ PB_REQ_BASE + 2000 作为调用其他接口的预留消息
#define		PB_BLOCK_MSG_BEGIN					(PB_REQ_BASE +   1000) // 起始

#define		PB_BLOCK_LV1_BEGIN					(PB_REQ_BASE +   1000) // 起始
#define		PB_HTTP_LV1_BEGIN					(PB_REQ_BASE +   1000) // 起始
#define		PB_GET_GAMELIST						(PB_REQ_BASE +   1001) // 获取合集大厅游戏更新列表
#define		PB_CHECK_UPDATE						(PB_REQ_BASE +   1002) // 检查更新
#define		PB_GET_MONEY_OWN					(PB_REQ_BASE +   1003) // 获取自身的财富信息
#define		PB_GET_GET_SIGNIN_INFO				(PB_REQ_BASE +   1004) // 获取签到信息
#define		PB_GET_GET_SIGNIN_AWARD				(PB_REQ_BASE +   1005) // 获取签到奖励
#define		PB_GET_ADVERTISMENT_ATTEND			(PB_REQ_BASE +   1006) // 参与广告活动
#define		PB_GET_ADVERTISMENT_GETAWARD		(PB_REQ_BASE +   1007) // 获取广告奖励
#define		PB_GET_ADVERTISMENT_GETINFO			(PB_REQ_BASE +   1008) // 获取广告活动详情
#define		PB_GET_REGISTER_GETAWARDINFO		(PB_REQ_BASE +   1009) // 新用户注册领取新手奖励
#define		PB_GET_INVITE_GETINFO				(PB_REQ_BASE +   1010) // 获取邀请信息（玩家自己的）
#define		PB_GET_INVITE_ATTEND				(PB_REQ_BASE +   1011) // 参与邀请有礼活动
#define		PB_GET_INVITE_GETAWARD				(PB_REQ_BASE +   1012) // 获取邀请奖励
#define		PB_GET_GIFTPACKAGE_GETAWARD			(PB_REQ_BASE +   1013) // 领取礼包奖励
#define		PB_GET_UPDATE_CONFIG				(PB_REQ_BASE +   1014) // 获取更新配置（用户客户端界面展示）

#define		PB_GET_BANKPROXY_BANKBALANCE		(PB_REQ_BASE +   1044) // 查询银行账户余额
#define		PB_GET_BANKPROXY_EXPIREINFO			(PB_REQ_BASE +   1045) // 查询银行账户过期信息
#define		PB_GET_PAYPROD_LIST					(PB_REQ_BASE +   1046) // 获取商品列表
#define		PB_GET_BANKPROXY_WITHDRAW			(PB_REQ_BASE +   1047) // 银行转账
#define		PB_HTTP_LV1_END						(PB_REQ_BASE +   1049) // 一级HTTP截止消息号
#define		PB_BLOCK_LV1_END					(PB_REQ_BASE +   1099) // 一级阻塞业务截止消息号

#define		PB_BLOCK_MSG_END					(PB_REQ_BASE +   1999) // 结束
//////////////////////////////////////////////////////////////////////////

#define		PB_REQ_END							(PB_REQ_BASE + 9999)  //增加的定义必须小于这个值


