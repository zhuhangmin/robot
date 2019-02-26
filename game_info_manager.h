#pragma once
#include "robot_define.h"
#include "table.h"
class GameInfoManager : public ISingletion<GameInfoManager> {
public:
    int Init(const std::string game_ip, const int game_port);

    int Term();

protected:
    SINGLETION_CONSTRUCTOR(GameInfoManager);

private:
    // ��Ϸ ��������
    int ConnectInfoGame(const std::string& game_ip, const int game_port);

    // ��Ϸ ��Ϣ����
    int SendGameRequest(const RequestID requestid, const google::protobuf::Message &val, REQUEST& response, const bool bNeedEcho = true);

    // ��Ϸ ��Ϣ����
    int ThreadGameInfoNotify();

    // ��Ϸ ��Ϣ����
    int OnGameInfoNotify(const RequestID requestid, const REQUEST &request);

    // ��Ϸ �Ͽ�����
    int OnDisconnGameInfo();

    // ��Ϸ ��ʱ����
    int ThreadSendGamePluse();

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
    int OnPlayerEnterGame(const REQUEST &request);

    // �Թ��߽�����Ϸ	BindLooker
    int OnLookerEnterGame(const REQUEST &request);

    // �Թ�ת���	BindPlayer
    int OnLooker2Player(const REQUEST &request);

    // ���ת�Թ�	UnbindPlayer
    int OnPlayer2Looker(const REQUEST &request);

    // ��ʼ��Ϸ	StartGame
    int OnStartGame(const REQUEST &request);

    // �û����˽���	RefreshGameResult(int userid)
    int OnUserFreshResult(const REQUEST &request);

    // ��������	RefreshGameResult
    int OnFreshResult(const REQUEST &request);

    // �û��뿪��Ϸ	UnbindUser
    int OnLeaveGame(const REQUEST &request);

    // �û�����	UnbindUser+BindPlaye
    int OnSwitchTable(const REQUEST &request);

private:
    //int GetUserStatus(UserID userid, UserStatus& user_status);
    int AddRoomPB(const game::base::Room room_pb);

    int AddTablePB(const game::base::Table table_pb, std::shared_ptr<Table> table);

    int AddUserPB(const game::base::User user_pb);

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
