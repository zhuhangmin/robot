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
    // ��Ϸ ��������
    int ConnectGame();

    // ��Ϸ ��Ϣ����
    int SendGameRequest(RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool bNeedEcho = true);

    // ��Ϸ ��Ϣ����
    void ThreadGameInfoNotify();

    // ��Ϸ ��Ϣ����
    void OnGameInfoNotify(RequestID nReqstID, const REQUEST &request);

    // ��Ϸ �Ͽ�����
    void OnDisconnGameInfo();

    // ��Ϸ ��ʱ����
    void ThreadSendGamePluse();

private:
    // ����ҵ����Ϣ

    // ����Ϸ������ ע�� Ϊ�Ϸ������˷�����
    int SendValidateReq();

    // ����Ϸ������ ��ȡ ��Ϸ��������Ϣ (room��table��user)
    int SendGetGameInfo(RoomID roomid = 0);

private:
    // ����ҵ����Ϣ

    // ������Ϸ������ ״̬֪ͨ
    void OnRecvGameStatus(const REQUEST &request);

private:
    int GetUserStatus(UserID userid, UserStatus& user_status);
    int FindTable(UserID userid, game::base::Table& table);
    int FindChair(UserID userid, game::base::ChairInfo& chair);

    int AddRoom(game::base::Room room_pb);

    int AddUser(game::base::User user_pb);
private:
    //���� ��Ϸ��������Ϣ �߳�
    UThread	game_info_notify_thread_;

    //���� ��ʱ�� �߳�
    UThread	heart_timer_thread_;

    //��Ϸ����������
    std::mutex game_info_connection_mutex_;
    CDefSocketClientPtr game_info_connection_{std::make_shared<CDefSocketClient>()};

    //���з�������
    RoomMap room_map_;

    //�����û�����
    UserMap user_map_;

};

