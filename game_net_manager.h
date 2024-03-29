#pragma once
#include "robot_define.h"
#include "table.h"
#include "room_manager.h"
#include "user_manager.h"
class GameNetManager : public ISingletion<GameNetManager> {
public:
    int Init(const std::string& game_ip, const int& game_port);

    int Term();

    // 游戏 是否连接
    int IsConnected(BOOL& is_connected) const;

    // 游戏数据层是否初始化完毕  允许脏读
    bool IsGameDataInited() const;

    // 桌银区间
    int GetTableDepositRange(const RoomID& roomid, const TableNO& tableno, int64_t& max, int64_t& min);

    // 所有房银区间 //TODO 提供特定房间的接口
    int GetRoomDepositRange(int64_t& max, int64_t& min) const;

    // 桌子上机器人数量
    int GetRobotCountOnChairs(const RoomID& roomid, const TableNO& tableno, int& count) const;

    int GetChairInfo(const RoomID& roomid, const TableNO& tableno, const int& userid, ChairInfo& info) const;

    int IsTablePlaying(const RoomID& roomid, const TableNO& tableno, bool& result) const;

    // 桌子上非机器人数量
    int GetNormalCountOnChairs(const RoomID& roomid, const TableNO& tableno, int& count) const;

    // 最小开桌人数
    int GetMiniPlayers(const RoomID& roomid, int& mini) const;

    // 机器人用户是否在游戏
    int IsRobotUserExist(const UserID& userid) const;

    // 获取所有正常用户集合 返回copy 不返回被mutex保护对象引用
    int GetNormalUserMap(UserMap& normal_user_map) const;

    // 获取所有的用户 返回copy 不返回被mutex保护对象引用
    UserMap GetAllUsers() const;

private:
    // 游戏 定时消息
    int ThreadTimer();

    // 游戏 消息接收
    int ThreadNotify();

    // 游戏 消息处理
    int OnNotify(const RequestID& requestid, const REQUEST &request);

    // 游戏 断开连接
    int OnDisconnect();

private:
    // 游戏 消息发送
    int SendRequestWithLock(const RequestID& requestid, const google::protobuf::Message &val, REQUEST& response, const bool& need_echo = true) const;

    // 发送业务消息
    int SendPulse();

    // 连接保活
    int KeepConnection();

private:
    // 接收业务消息
    // 一般需要先更新user数据，触发table上的用户数据变化

    // 玩家进入游戏	BindPlayer
    int OnPlayerEnterGame(const REQUEST &request);

    // 旁观者进入游戏	BindLooker
    int OnLookerEnterGame(const REQUEST &request);

    // 旁观转玩家	BindPlayer
    int OnLooker2Player(const REQUEST &request) const;

    // 玩家转旁观	UnbindPlayer
    int OnPlayer2Looker(const REQUEST &request) const;

    // 开始游戏	StartGame
    int OnStartGame(const REQUEST &request) const;

    // 用户单人结算	RefreshGameResult(int userid)
    int OnUserFreshResult(const REQUEST &request) const;

    // 整桌结算	RefreshGameResult
    int OnFreshResult(const REQUEST &request) const;

    // 用户离开游戏	UnbindUser
    int OnLeaveGame(const REQUEST &request);

    // 用户换桌	UnbindUser+BindPlaye
    int OnSwitchTable(const REQUEST &request) const;

    // 新房间创建
    int OnNewRoom(const REQUEST &request);

private:
    // 辅助函数 WithLock 标识调用前需要获得此对象的数据锁mutex

    // 游戏 初始化连接和数据
    int InitDataWithLock();

    // 游戏 重置连接和数据
    int ResetDataWithLock();

    // 游戏 建立连接
    int ConnectWithLock(const std::string& game_ip, const int& game_port) const;

    // 向游戏服务器 注册 为合法机器人服务器
    int SendValidateReqWithLock();

    // 向游戏服务器 获取 游戏内所有信息 (room、table、user)
    int SendGetGameInfoWithLock();

public:
    // 对象状态快照
    int SnapShotObjectStatus() const;

    int BriefInfo() const;

protected:
    SINGLETION_CONSTRUCTOR(GameNetManager);

private:
    //游戏服务器连接
    // 管理的数据层为 RoomManager, UserManager
    // 接收游戏服务器网络数据，上层逻辑业务层[不能修改] RoomManager, UserManager
    // 可以认为整个RoomManager, UserManager 为游戏服务器数据层[只读]快照
    mutable std::mutex mutex_;
    CDefSocketClientPtr connection_{std::make_shared<CDefSocketClient>()};
    int timeout_count_{0};

    // 接收 游戏服务器消息 线程
    YQThread notify_thread_;

    // 心跳 定时器 线程
    YQThread timer_thread_;

    // 兜底同步信息时间
    TimeStamp sync_time_stamp_{std::time(nullptr)};

    // 心跳时间戳
    TimeStamp timestamp_{std::time(nullptr)};

    // 游戏服务器 IP
    std::string game_ip_;

    // 游戏服务器 PORT
    int game_port_{InvalidPort};

    // 重连标签
    bool need_reconnect_{false};

    // 初始化游戏数据层标签
    bool game_data_inited_{false};

    // 游戏服务器所有用户
    UserManager user_mgr_;

    // 游戏服务器所有房间
    RoomManager room_mgr_;
};

#define GameMgr GameNetManager::Instance()
