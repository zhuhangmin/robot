#pragma once
#include "robot_define.h"
#include "table.h"
class GameInfoManager : public ISingletion<GameInfoManager> {
public:
    int Init(std::string game_ip, int game_port);

    void Term();

protected:
    SINGLETION_CONSTRUCTOR(GameInfoManager);

private:
    // 游戏 建立连接
    int ConnectInfoGame(std::string game_ip, int game_port);

    // 游戏 消息发送
    int SendGameRequest(RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool bNeedEcho = true);

    // 游戏 消息接收
    void ThreadGameInfoNotify();

    // 游戏 消息处理
    int OnGameInfoNotify(RequestID requestid, const REQUEST &request);

    // 游戏 断开连接
    void OnDisconnGameInfo();

    // 游戏 定时心跳
    void ThreadSendGamePluse();

private:
    // 发送业务消息

    // 向游戏服务器 注册 为合法机器人服务器
    int SendValidateReq();

    // 向游戏服务器 获取 游戏内所有信息 (room、table、user)
    int SendGetGameInfo();

    // 向游戏服务器 发送 心跳
    int SendPulse();

private:
    // 接收业务消息 一般需要先更新user数据，在触发table上的用户数据变化

    // 玩家进入游戏	BindPlayer
    void OnPlayerEnterGame(const REQUEST &request);

    // 旁观者进入游戏	BindLooker
    void OnLookerEnterGame(const REQUEST &request);

    // 旁观转玩家	BindPlayer
    void OnLooker2Player(const REQUEST &request);

    // 玩家转旁观	UnbindPlayer
    void OnPlayer2Looker(const REQUEST &request);

    // 开始游戏	StartGame
    void OnStartGame(const REQUEST &request);

    // 用户单人结算	RefreshGameResult(int userid)
    void OnUserFreshResult(const REQUEST &request);

    // 整桌结算	RefreshGameResult
    void OnFreshResult(const REQUEST &request);

    // 用户离开游戏	UnbindUser
    void OnLeaveGame(const REQUEST &request);

    // 用户换桌	UnbindUser+BindPlaye
    void OnSwitchTable(const REQUEST &request);

private:
    //int GetUserStatus(UserID userid, UserStatus& user_status);
    int AddRoomPB(game::base::Room room_pb);

    int AddTablePB(game::base::Table table_pb, std::shared_ptr<Table> table);

    int AddUserPB(game::base::User user_pb);

private:
    //接收 游戏服务器消息 线程
    YQThread	game_info_notify_thread_;

    //心跳 定时器 线程
    YQThread	heart_timer_thread_;

    //游戏服务器连接
    std::mutex game_info_connection_mutex_;
    CDefSocketClientPtr game_info_connection_{std::make_shared<CDefSocketClient>()};

};

#define GameMgr GameInfoManager::Instance()
