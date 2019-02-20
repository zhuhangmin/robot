#pragma once
#include "RobotDef.h"



class Robot {
public:
    Robot();
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
    DepositType GetGainType() const { return gain_type_; }
    void SetGainType(DepositType val) { gain_type_ = val; }
private:
    int ConnectGameWithLock(const std::string& strIP, const int32_t nPort, uint32_t nThrdId);
    int SendRequestWithLock(RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool bNeedEcho = true);

    DepositType gain_type_{DepositType::kDefault};

protected:
    int32_t			userid_{0};
    std::string		password_;
    LOGON_SUCCEED_V2   m_LogonData{};
    CDefSocketClientPtr game_connection_{std::make_shared<CDefSocketClient>()};

public:
    //zhuhangmin 20181019  mutex_ 一把锁管理机器人的所有状态,目前没有需要细化数据颗粒锁的必要
    std::mutex		mutex_;

    int32_t			m_nWantRoomId{};

    // 是否要取银
    bool			m_bGainDeposit{false};
    int64_t			m_nGainAmount{};

    // 是否要存银
    bool			m_bBackDeposit{false};
    int32_t			m_nBackAmount{};

    bool		logon_{false}; // 是否登入大厅


};

using RobotPtr = std::shared_ptr<Robot>;