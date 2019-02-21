#pragma once
#include "RobotDef.h"
class GameInfoManager : public ISingletion<GameInfoManager> {
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

    // ��ҽ�����Ϸ	BindPlayer
    void OnPlayerEnterGame(const REQUEST &request);

    // �Թ��߽�����Ϸ	BindLooker
    void OnLookerEnterGame(const REQUEST &request);

    // �Թ�ת���	BindPlayer
    void OnLooker2Player(const REQUEST &request);

    // ���ת�Թ�	UnbindPlayer
    void OnPlayer2Looker(const REQUEST &request);

    // ��ʼ��Ϸ	StartGame
    void OnStartGame(const REQUEST &request);

    // �û����˽���	RefreshGameResult(int userid)
    void OnUserFreshResult(const REQUEST &request);

    // ��������	RefreshGameResult
    void OnFreshResult(const REQUEST &request);

    // �û��뿪��Ϸ	UnbindUser
    void OnLeaveGame(const REQUEST &request);

    // �û�����	UnbindUser+BindPlaye
    void OnSwitchGame(const REQUEST &request);

private:
    //int GetUserStatus(UserID userid, UserStatus& user_status);
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


};

