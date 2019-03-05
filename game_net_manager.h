#pragma once
#include "robot_define.h"
#include "table.h"
class GameNetManager : public ISingletion<GameNetManager> {
public:
    int Init(const std::string& game_ip, const int& game_port);

    int Term();

private:
    // 游戏 消息发送
    int SendGameRequest(RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool bNeedEcho = true) const;

    // 游戏 消息接收
    int ThreadGameInfoNotify();

    // 游戏 消息处理
    int OnGameInfoNotify(RequestID requestid, const REQUEST &request);

    // 游戏 断开连接
    int OnDisconnGameInfo();

    // 游戏 定时心跳
    int ThreadSendGamePulse();

private:
    // 发送业务消息
    int SendPulse();

private:
    // 接收业务消息
    // 一般需要先更新user数据，触发table上的用户数据变化

    // 玩家进入游戏	BindPlayer
    int OnPlayerEnterGame(const REQUEST &request) const;

    // 旁观者进入游戏	BindLooker
    int OnLookerEnterGame(const REQUEST &request) const;

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
    int OnLeaveGame(const REQUEST &request) const;

    // 用户换桌	UnbindUser+BindPlaye
    int OnSwitchTable(const REQUEST &request) const;

private:
    //int GetUserStatus(UserID userid, UserStatus& user_status);
    int AddRoomPB(const game::base::Room& room_pb) const;

    static int AddTablePB(const game::base::Table& table_pb, const TablePtr& table);

    int AddUserPB(const game::base::User& user_pb) const;

private:
    // 辅助函数 WithLock 标识调用前需要获得此对象的数据锁mutex

    // 游戏 初始化连接和数据
    int InitDataWithLock();

    // 游戏 重置连接和数据
    int ResetDataWithLock();

    // LEVEL 2 : ResetDataWithLock + InitDataWithLock
    int ResetInitDataWithLock();

    // 游戏 建立连接
    int ConnectGameSvrWithLock(const std::string& game_ip, int game_port) const;

    // 向游戏服务器 注册 为合法机器人服务器
    int SendValidateReqWithLock();

    // 向游戏服务器 获取 游戏内所有信息 (room、table、user)
    int SendGetGameInfoWithLock();

public:
    // 对象状态快照
    int SnapShotObjectStatus();

private:
    // 方法的线程可见性检查
    int CheckNotInnerThread();

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
    YQThread heart_thread_;

    // 游戏服务器 IP
    std::string game_ip_;

    // 游戏服务器 PORT
    int game_port_{InvalidPort};
};

#define GameMgr GameNetManager::Instance()
