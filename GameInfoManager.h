#pragma once
#include "RobotDef.h"
class GameInfoManager : public ISingletion<GameInfoManager> {
public:
    using RoomMap = std::unordered_map<RoomID, game::base::Room>;

    using UserMap = std::unordered_map<UserID, game::base::User>;

public:
    int Init();

    void Term();


protected:
    SINGLETION_CONSTRUCTOR(GameInfoManager);

private:
    // 游戏 建立连接
    int ConnectGame();

    // 游戏 消息发送
    int SendGameRequest(RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool bNeedEcho = true);

    // 游戏 消息接收
    void ThreadGameInfoNotify();

    // 游戏 消息处理
    void OnGameInfoNotify(RequestID nReqstID, const REQUEST &request);

    // 游戏 断开连接
    void OnDisconnGameInfo();

    // 游戏 定时心跳
    void ThreadSendGamePluse();

private:
    // 发送业务消息

    // 向游戏服务器 注册 为合法机器人服务器
    int SendValidateReq();

    // 向游戏服务器 获取 游戏内所有信息 (room、table、user)
    int SendGetGameInfo(RoomID roomid = 0);

private:
    // 接收业务消息

    // 接收游戏服务器 状态通知
    void OnRecvGameStatus(const REQUEST &request);

private:
    int GetUserStatus(UserID userid, UserStatus& user_status);
    int FindTable(UserID userid, game::base::Table& table);
    int FindChair(UserID userid, game::base::ChairInfo& chair);

    int AddRoom(game::base::Room room_pb);

    int AddUser(game::base::User user_pb);
private:
    //接收 游戏服务器消息 线程
    UThread	game_info_notify_thread_;

    //心跳 定时器 线程
    UThread	heart_timer_thread_;

    //游戏服务器连接
    std::mutex game_info_connection_mutex_;
    CDefSocketClientPtr game_info_connection_{std::make_shared<CDefSocketClient>()};

    //所有房间数据
    RoomMap room_map_;

    //所有用户数据
    UserMap user_map_;

};

