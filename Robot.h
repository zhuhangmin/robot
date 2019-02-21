#pragma once
#include "RobotDef.h"



class Robot {
public:
    Robot();
    Robot(UserID userid);

public:
    // 游戏 连接
    int ConnectGame(const std::string& strIP, const int32_t nPort, uint32_t nThrdId);

    // 游戏 断开
    void OnDisconnect();

public:
    // 具体业务

    // 进入游戏
    int SendEnterGame(RoomID roomid, uint32_t nNofifyThrId, std::string sNick, std::string sPortr, int nTableNo, int nChairNo);

    // 发送心跳
    void SendGamePulse();

public:
    // 属性接口
    int32_t	GetUserID();

    TokenID	GetTokenID();

    DepositType GetGainType() const;

    void SetGainType(DepositType val);

    // 大厅 登陆数据
    void SetLogonData(LPLOGON_SUCCEED_V2 logonOk);

    bool IsLogon();

    void SetLogon(bool status);

private:

    int SendGameRequest(RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool bNeedEcho = true);

private:
    // 配置机器人ID
    UserID userid_{0};

    // 是否登入大厅
    bool logon_{false};

    // 登陆数据
    LOGON_SUCCEED_V2   m_LogonData{};

    // 补银类型
    DepositType gain_type_{DepositType::kDefault};

    // 游戏连接
    std::mutex mutex_;
    CDefSocketClientPtr game_connection_{std::make_shared<CDefSocketClient>()};

};

using RobotPtr = std::shared_ptr<Robot>;