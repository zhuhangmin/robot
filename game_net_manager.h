#pragma once
#include "robot_define.h"
#include "table.h"
class GameNetManager : public ISingletion<GameNetManager> {
public:
    int Init(const std::string game_ip, const int game_port);

    int Term();

protected:
    SINGLETION_CONSTRUCTOR(GameNetManager);

private:
    // 游戏 消息发送
    int SendGameRequest(const RequestID requestid, const google::protobuf::Message &val, REQUEST& response, const bool bNeedEcho = true);

    // 游戏 消息接收
    int ThreadGameInfoNotify();

    // 游戏 消息处理
    int OnGameInfoNotify(const RequestID requestid, const REQUEST &request);

    // 游戏 断开连接
    int OnDisconnGameInfo();

    // 游戏 定时心跳
    int ThreadSendGamePulse();

private:
    // 发送业务消息

    // 向游戏服务器 发送 心跳
    int SendPulse();

private:
    // 接收业务消息 一般需要先更新user数据，在触发table上的用户数据变化

    // 玩家进入游戏	BindPlayer
    int OnPlayerEnterGame(const REQUEST &request);

    // 旁观者进入游戏	BindLooker
    int OnLookerEnterGame(const REQUEST &request);

    // 旁观转玩家	BindPlayer
    int OnLooker2Player(const REQUEST &request);

    // 玩家转旁观	UnbindPlayer
    int OnPlayer2Looker(const REQUEST &request);

    // 开始游戏	StartGame
    int OnStartGame(const REQUEST &request);

    // 用户单人结算	RefreshGameResult(int userid)
    int OnUserFreshResult(const REQUEST &request);

    // 整桌结算	RefreshGameResult
    int OnFreshResult(const REQUEST &request);

    // 用户离开游戏	UnbindUser
    int OnLeaveGame(const REQUEST &request);

    // 用户换桌	UnbindUser+BindPlaye
    int OnSwitchTable(const REQUEST &request);

private:
    //int GetUserStatus(UserID userid, UserStatus& user_status);
    int AddRoomPB(const game::base::Room room_pb);

    int AddTablePB(const game::base::Table table_pb, TablePtr table);

    int AddUserPB(const game::base::User user_pb);

private:
    // 辅助函数 WithLock 标识调用前需要获得此对象的数据锁mutex

    // 游戏 初始化连接和数据
    int InitDataWithLock();

    // 游戏 重置连接和数据
    int ResetDataWithLock();

    // LEVEL 2 : ResetDataWithLock + InitDataWithLock
    int ResetInitDataWithLock();

    // 游戏 建立连接
    int ConnectGameSvrWithLock(const std::string& game_ip, const int game_port);

    // 向游戏服务器 注册 为合法机器人服务器
    int SendValidateReqWithLock();

    // 向游戏服务器 获取 游戏内所有信息 (room、table、user)
    int SendGetGameInfoWithLock();

    // 游戏 断开连接
    int OnDisconnGameInfoWithLock();

public:
    // 对象状态快照
    int SnapShotObjectStatus();

    // 获得房间内特定状态人数

private:
    // 接收 游戏服务器消息 线程
    YQThread	game_info_notify_thread_;

    // 心跳 定时器 线程
    YQThread	heart_timer_thread_;

    // 游戏服务器 IP
    std::string game_ip_;

    // 游戏服务器 PORT
    int game_port_;

    //游戏服务器连接
    mutable std::mutex game_info_connection_mutex_;
    CDefSocketClientPtr game_info_connection_{std::make_shared<CDefSocketClient>()};
    // 管理的数据层为 room_manager 和 user_manager 
    // 只用来接收网络消息形成数据层，逻辑业务层[不能修改] room_manager 和 user_manager
    // 可以认为整个room_manager 和 user_manager 为游戏服务器数据层[只读]快照


};

#define GameMgr GameNetManager::Instance()
