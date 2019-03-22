#pragma once
#include "robot_define.h"
#include "table.h"
class GameNetManager : public ISingletion<GameNetManager> {
public:
    int Init(const std::string& game_ip, const int& game_port);

    int Term();

    // ��Ϸ �Ƿ�����
    int IsConnected(BOOL& is_connected) const;

    // ��Ϸ���ݲ��Ƿ��ʼ�����  �������
    bool IsGameDataInited() const;

    // ��������
    int GetTableDepositRange(const RoomID& roomid, const TableNO& tableno, int64_t& max, int64_t& min);

    // ���з������� //TODO �ṩ�ض�����Ľӿ�
    int GetRoomDepositRange(int64_t& max, int64_t& min) const;

    // �����ϻ���������
    int GetRobotCountOnChairs(const RoomID& roomid, const TableNO& tableno, int& count) const;

    int GetChairInfo(const RoomID& roomid, const TableNO& tableno, const int& userid, ChairInfo& info) const;

    int IsTablePlaying(const RoomID& roomid, const TableNO& tableno, bool& result) const;

    // �����Ϸǻ���������
    int GetNormalCountOnChairs(const RoomID& roomid, const TableNO& tableno, int& count) const;

    // ��С��������
    int GetMiniPlayers(const RoomID& roomid, int& mini) const;

    // �������û��Ƿ�����Ϸ
    int IsRobotUserExist(const UserID& userid) const;

    // ��ȡ���������û����� ����copy �����ر�mutex������������
    int GetNormalUserMap(UserMap& normal_user_map) const;

private:
    // ��Ϸ ��ʱ��Ϣ
    int ThreadTimer();

    // ��Ϸ ��Ϣ����
    int ThreadNotify();

    // ��Ϸ ��Ϣ����
    int OnNotify(const RequestID& requestid, const REQUEST &request);

    // ��Ϸ �Ͽ�����
    int OnDisconnect();

private:
    // ��Ϸ ��Ϣ����
    int SendRequestWithLock(const RequestID& requestid, const google::protobuf::Message &val, REQUEST& response, const bool& need_echo = true) const;

    // ����ҵ����Ϣ
    int SendPulse();

    // ���ӱ���
    int KeepConnection();

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

    // �·��䴴��
    int OnNewRoom(const REQUEST &request) const;

private:
    // �������� WithLock ��ʶ����ǰ��Ҫ��ô˶����������mutex

    // ��Ϸ ��ʼ�����Ӻ�����
    int InitDataWithLock();

    // ��Ϸ �������Ӻ�����
    int ResetDataWithLock();

    // ��Ϸ ��������
    int ConnectWithLock(const std::string& game_ip, const int& game_port) const;

    // ����Ϸ������ ע�� Ϊ�Ϸ������˷�����
    int SendValidateReqWithLock();

    // ����Ϸ������ ��ȡ ��Ϸ��������Ϣ (room��table��user)
    int SendGetGameInfoWithLock();

public:
    // ����״̬����
    int SnapShotObjectStatus() const;

    int BriefInfo() const;

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
    YQThread timer_thread_;

    // ����ͬ����Ϣʱ��
    TimeStamp sync_time_stamp_{std::time(nullptr)};

    // ����ʱ���
    TimeStamp timestamp_{std::time(nullptr)};

    // ��Ϸ������ IP
    std::string game_ip_;

    // ��Ϸ������ PORT
    int game_port_{InvalidPort};

    // ������ǩ
    bool need_reconnect_{false};

    // ��ʼ����Ϸ���ݲ��ǩ
    bool game_data_inited_{false};
};

#define GameMgr GameNetManager::Instance()
