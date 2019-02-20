#pragma once
#include "RobotDef.h"



class Robot {
public:
    Robot(const RobotSetting& robot);
    virtual ~Robot();

public:
    int32_t		GetUserID() { return userid_; }
    std::string	Password() { return password_; }
    TokenID	GameToken() { return game_connection_->GetTokenID(); }

    //HALL
    void		SetLogonData(LPLOGON_SUCCEED_V2 logonOk) { m_LogonData = *logonOk; }
    bool        IsLogon() { return logon_; }
    void        SetLogon(bool status) { logon_ = status; }

    void		OnDisconnGame();

    // for Send
    int SendEnterGame(const ROOM& room, uint32_t nNofifyThrId, std::string sNick, std::string sPortr, int nTableNo, int nChairNo);
    int SendGamePulse();

    void SetPlayerRoomStatus(int status);
private:
    bool ConnectGameWithLock(const std::string& strIP, const int32_t nPort, uint32_t nThrdId);
    int SendRequestWithLock(RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool bNeedEcho = true);


protected:
    int32_t			userid_{0};
    std::string		password_;
    LOGON_SUCCEED_V2   m_LogonData{};
    CDefSocketClientPtr game_connection_{std::make_shared<CDefSocketClient>()};

public:
    //zhuhangmin 20181019  mutex_ һ������������˵�����״̬,Ŀǰû����Ҫϸ�����ݿ������ı�Ҫ
    std::mutex		mutex_;

    int32_t			m_nWantRoomId{};

    // �Ƿ�Ҫȡ��
    bool			m_bGainDeposit{false};
    int64_t			m_nGainAmount{};

    // �Ƿ�Ҫ����
    bool			m_bBackDeposit{false};
    int32_t			m_nBackAmount{};

    bool		logon_{false}; // �Ƿ�������

};

using RobotPtr = std::shared_ptr<Robot>;