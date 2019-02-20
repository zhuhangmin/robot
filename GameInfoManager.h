#pragma once
#include "RobotDef.h"
class GameInfoManager : public ISingletion<GameInfoManager> {
public:
    using RoomMap = std::unordered_map<RoomID, game::base::Room>;
    using UserMap = std::unordered_map<UserID, game::base::User>;
public:
    // ��ʼ|����
    int Init();
    void Term();


public:
    int GetUserStatus(UserID userid, UserStatus& user_status);
    int FindTable(UserID userid, game::base::Table& table);
    int FindChair(UserID userid, game::base::ChairInfo& chair);

protected:
    SINGLETION_CONSTRUCTOR(GameInfoManager);

    int InitConnectGame();

    int SendValidateReq();
    int SendGetGameInfo(RoomID roomid = 0);

    void	ThreadRunGameInfoNotify();
    void OnGameInfoNotify(RequestID nReqstID, const REQUEST &request);
    //TODO KEEPALIVE THREAD
private:
    void OnDisconnGameInfo();
    void OnRecvGameInfo(const REQUEST &request);
private:
    UThread			m_thrdGameInfoNotify;
    std::mutex game_info_connection_mutex_; // ����ͬ����Ϸ������������Ϣ
    CDefSocketClientPtr game_info_connection_{std::make_shared<CDefSocketClient>()};//��Ϸ����
    RoomMap room_map_;
    UserMap user_map_;

};

