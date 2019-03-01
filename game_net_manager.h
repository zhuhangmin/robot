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
    // ��Ϸ ��Ϣ����
    int SendGameRequest(const RequestID requestid, const google::protobuf::Message &val, REQUEST& response, const bool bNeedEcho = true);

    // ��Ϸ ��Ϣ����
    int ThreadGameInfoNotify();

    // ��Ϸ ��Ϣ����
    int OnGameInfoNotify(const RequestID requestid, const REQUEST &request);

    // ��Ϸ �Ͽ�����
    int OnDisconnGameInfo();

    // ��Ϸ ��ʱ����
    int ThreadSendGamePulse();

private:
    // ����ҵ����Ϣ

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

    int AddTablePB(const game::base::Table table_pb, TablePtr table);

    int AddUserPB(const game::base::User user_pb);

private:
    // �������� WithLock ��ʶ����ǰ��Ҫ��ô˶����������mutex

    // ��Ϸ ��ʼ�����Ӻ�����
    int InitDataWithLock();

    // ��Ϸ �������Ӻ�����
    int ResetDataWithLock();

    // LEVEL 2 : ResetDataWithLock + InitDataWithLock
    int ResetInitDataWithLock();

    // ��Ϸ ��������
    int ConnectGameSvrWithLock(const std::string& game_ip, const int game_port);

    // ����Ϸ������ ע�� Ϊ�Ϸ������˷�����
    int SendValidateReqWithLock();

    // ����Ϸ������ ��ȡ ��Ϸ��������Ϣ (room��table��user)
    int SendGetGameInfoWithLock();

    // ��Ϸ �Ͽ�����
    int OnDisconnGameInfoWithLock();

public:
    // ����״̬����
    int SnapShotObjectStatus();

    // ��÷������ض�״̬����

private:
    // ���� ��Ϸ��������Ϣ �߳�
    YQThread	game_info_notify_thread_;

    // ���� ��ʱ�� �߳�
    YQThread	heart_timer_thread_;

    // ��Ϸ������ IP
    std::string game_ip_;

    // ��Ϸ������ PORT
    int game_port_;

    //��Ϸ����������
    mutable std::mutex game_info_connection_mutex_;
    CDefSocketClientPtr game_info_connection_{std::make_shared<CDefSocketClient>()};
    // ��������ݲ�Ϊ room_manager �� user_manager 
    // ֻ��������������Ϣ�γ����ݲ㣬�߼�ҵ���[�����޸�] room_manager �� user_manager
    // ������Ϊ����room_manager �� user_manager Ϊ��Ϸ���������ݲ�[ֻ��]����


};

#define GameMgr GameNetManager::Instance()
