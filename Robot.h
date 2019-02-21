#pragma once
#include "RobotDef.h"



class Robot {
public:
    Robot();
    Robot(UserID userid);

public:
    // ��Ϸ ����
    int ConnectGame(const std::string& strIP, const int32_t nPort, uint32_t nThrdId);

    // ��Ϸ �Ͽ�
    void OnDisconnect();

public:
    // ����ҵ��

    // ������Ϸ
    int SendEnterGame(RoomID roomid, uint32_t nNofifyThrId, std::string sNick, std::string sPortr, int nTableNo, int nChairNo);

    // ��������
    void SendGamePulse();

public:
    // ���Խӿ�
    int32_t	GetUserID();

    TokenID	GetTokenID();

    DepositType GetGainType() const;

    void SetGainType(DepositType val);

    // ���� ��½����
    void SetLogonData(LPLOGON_SUCCEED_V2 logonOk);

    bool IsLogon();

    void SetLogon(bool status);

private:

    int SendGameRequest(RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool bNeedEcho = true);

private:
    // ���û�����ID
    UserID userid_{0};

    // �Ƿ�������
    bool logon_{false};

    // ��½����
    LOGON_SUCCEED_V2   m_LogonData{};

    // ��������
    DepositType gain_type_{DepositType::kDefault};

    // ��Ϸ����
    std::mutex mutex_;
    CDefSocketClientPtr game_connection_{std::make_shared<CDefSocketClient>()};

};

using RobotPtr = std::shared_ptr<Robot>;