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
    // ��Ϸ ��������
    int ConnectInfoGame(std::string game_ip, int game_port);

    // ��Ϸ ��Ϣ����
    int SendGameRequest(RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool bNeedEcho = true);

    // ��Ϸ ��Ϣ����
    void ThreadGameInfoNotify();

    // ��Ϸ ��Ϣ����
    int OnGameInfoNotify(RequestID requestid, const REQUEST &request);

    // ��Ϸ �Ͽ�����
    void OnDisconnGameInfo();

    // ��Ϸ ��ʱ����
    void ThreadSendGamePluse();

private:
    // ����ҵ����Ϣ

    // ����Ϸ������ ע�� Ϊ�Ϸ������˷�����
    int SendValidateReq();

    // ����Ϸ������ ��ȡ ��Ϸ��������Ϣ (room��table��user)
    int SendGetGameInfo();

    // ����Ϸ������ ���� ����
    int SendPulse();

private:
    // ����ҵ����Ϣ һ����Ҫ�ȸ���user���ݣ��ڴ���table�ϵ��û����ݱ仯

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
    void OnSwitchTable(const REQUEST &request);

private:
    //int GetUserStatus(UserID userid, UserStatus& user_status);
    int AddRoomPB(game::base::Room room_pb);

    int AddTablePB(game::base::Table table_pb, std::shared_ptr<Table> table);

    int AddUserPB(game::base::User user_pb);

private:
    //���� ��Ϸ��������Ϣ �߳�
    YQThread	game_info_notify_thread_;

    //���� ��ʱ�� �߳�
    YQThread	heart_timer_thread_;

    //��Ϸ����������
    std::mutex game_info_connection_mutex_;
    CDefSocketClientPtr game_info_connection_{std::make_shared<CDefSocketClient>()};

};

#define GameMgr GameInfoManager::Instance()
