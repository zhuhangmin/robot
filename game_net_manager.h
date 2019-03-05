#pragma once
#include "robot_define.h"
#include "table.h"
class GameNetManager : public ISingletion<GameNetManager> {
public:
    int Init(const std::string& game_ip, const int& game_port);

    int Term();

private:
    // ��Ϸ ��Ϣ����
    int SendGameRequest(RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool bNeedEcho = true) const;

    // ��Ϸ ��Ϣ����
    int ThreadGameInfoNotify();

    // ��Ϸ ��Ϣ����
    int OnGameInfoNotify(RequestID requestid, const REQUEST &request);

    // ��Ϸ �Ͽ�����
    int OnDisconnGameInfo();

    // ��Ϸ ��ʱ����
    int ThreadSendGamePulse();

private:
    // ����ҵ����Ϣ
    int SendPulse();

private:
    // ����ҵ����Ϣ
    // һ����Ҫ�ȸ���user���ݣ�����table�ϵ��û����ݱ仯

    // ��ҽ�����Ϸ	BindPlayer
    int OnPlayerEnterGame(const REQUEST &request) const;

    // �Թ��߽�����Ϸ	BindLooker
    int OnLookerEnterGame(const REQUEST &request) const;

    // �Թ�ת���	BindPlayer
    int OnLooker2Player(const REQUEST &request) const;

    // ���ת�Թ�	UnbindPlayer
    int OnPlayer2Looker(const REQUEST &request) const;

    // ��ʼ��Ϸ	StartGame
    int OnStartGame(const REQUEST &request) const;

    // �û����˽���	RefreshGameResult(int userid)
    int OnUserFreshResult(const REQUEST &request) const;

    // ��������	RefreshGameResult
    int OnFreshResult(const REQUEST &request) const;

    // �û��뿪��Ϸ	UnbindUser
    int OnLeaveGame(const REQUEST &request) const;

    // �û�����	UnbindUser+BindPlaye
    int OnSwitchTable(const REQUEST &request) const;

private:
    //int GetUserStatus(UserID userid, UserStatus& user_status);
    int AddRoomPB(const game::base::Room& room_pb) const;

    static int AddTablePB(const game::base::Table& table_pb, const TablePtr& table);

    int AddUserPB(const game::base::User& user_pb) const;

private:
    // �������� WithLock ��ʶ����ǰ��Ҫ��ô˶����������mutex

    // ��Ϸ ��ʼ�����Ӻ�����
    int InitDataWithLock();

    // ��Ϸ �������Ӻ�����
    int ResetDataWithLock();

    // LEVEL 2 : ResetDataWithLock + InitDataWithLock
    int ResetInitDataWithLock();

    // ��Ϸ ��������
    int ConnectGameSvrWithLock(const std::string& game_ip, int game_port) const;

    // ����Ϸ������ ע�� Ϊ�Ϸ������˷�����
    int SendValidateReqWithLock();

    // ����Ϸ������ ��ȡ ��Ϸ��������Ϣ (room��table��user)
    int SendGetGameInfoWithLock();

public:
    // ����״̬����
    int SnapShotObjectStatus();

private:
    // �������߳̿ɼ��Լ��
    int CheckNotInnerThread();

protected:
    SINGLETION_CONSTRUCTOR(GameNetManager);

private:
    //��Ϸ����������
    // ��������ݲ�Ϊ RoomManager, UserManager
    // ������Ϸ�������������ݣ��ϲ��߼�ҵ���[�����޸�] RoomManager, UserManager
    // ������Ϊ����RoomManager, UserManager Ϊ��Ϸ���������ݲ�[ֻ��]����
    mutable std::mutex mutex_;
    CDefSocketClientPtr connection_{std::make_shared<CDefSocketClient>()};
    int timeout_count_{0};

    // ���� ��Ϸ��������Ϣ �߳�
    YQThread notify_thread_;

    // ���� ��ʱ�� �߳�
    YQThread heart_thread_;

    // ��Ϸ������ IP
    std::string game_ip_;

    // ��Ϸ������ PORT
    int game_port_{InvalidPort};
};

#define GameMgr GameNetManager::Instance()
