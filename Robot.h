#pragma once
#include "RobotDef.h"



class Robot {
public:
    Robot(const RobotSetting& robot);
    virtual ~Robot();

public:
    int32_t		GetUserID() { return userid_; }
    std::string	Password() { return m_Password; }
    int32_t		GetRoomID() { return roomid_; }
    bool		IsGaming() { return m_bRunGame; }
    void		SetGaming(bool isGame) { m_bRunGame = isGame; }
    TokenID	RoomToken() { return connection_room_->GetTokenID(); }
    TokenID	GameToken() { return connection_game_->GetTokenID(); }

    void		SetLogonData(LPLOGON_SUCCEED_V2 logonOk) { m_LogonData = *logonOk; }

    bool        IsLogon() { return logon_; }

    void        SetLogon(bool status) { logon_ = status; }

    bool IsInRoom() { return roomid_ != 0; }
    void SetRoomID(RoomID roomid) { roomid_ = roomid; }


    void		OnDisconnRoom();
    void		OnDisconnGame();

    // for Room


    // for Game



    // for Send
    TTueRet		SendEnterRoom(const ROOM& room, uint32_t nNofifyThrId);
    int SendEnterGame(const ROOM& room, uint32_t nNofifyThrId, std::string sNick, std::string sPortr, int nTableNo, int nChairNo);
    //@zhuhangmin 20181017
    TTueRet		SendGetNewTable(const ROOM& room, uint32_t nNofifyThrId, NTF_GET_NEWTABLE& lpNewTableInfo);


    TTueRet		SendRoomPulse();
    TTueRet		SendGamePulse();

    int GetPlayerRoomStatus();

    void SetPlayerRoomStatus(int status);
private:
    bool ConnectRoomWithLock(const std::string& strIP, const int32_t nPort, uint32_t nThrdId); //caller must require with lock
    bool ConnectGameWithLock(const std::string& strIP, const int32_t nPort, uint32_t nThrdId);
    TTueRet SendRoomRequestWithLock(RequestID nReqId, uint32_t& nDataLen, void *pData, RequestID &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho = true, uint32_t wait_ms = REQ_TIMEOUT_INTERVAL);
    //TTueRet SendRequestWithLock(RequestID nReqId, uint32_t& nDataLen, void *pData, RequestID &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho = true, uint32_t wait_ms = REQ_TIMEOUT_INTERVAL);
private://@zhuhangmin 
    int SendRequestWithLock(RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool bNeedEcho = true);


protected:
    int32_t			userid_{0};
    std::string		m_Password;
    int32_t			roomid_{};
    bool			m_bRunGame{false};
    LOGON_SUCCEED_V2   m_LogonData{};
    ENTER_ROOM_OK   m_EnterRoomData{};
    PLAYER			m_PlayerData{};

    CDefSocketClientPtr connection_room_{std::make_shared<CDefSocketClient>()};

    CDefSocketClientPtr connection_game_{std::make_shared<CDefSocketClient>()};

public:
    // 游戏的银子
    int32_t			m_nWantRoomId{};

    // 是否要取银
    bool			m_bGainDeposit{false};
    int64_t			m_nGainAmount{};

    // 是否要存银
    bool			m_bBackDeposit{false};
    int32_t			m_nBackAmount{};

    // 延迟Enter
    int32_t			m_nToTable{0};
    int32_t			m_nToChair{0};
    std::string		m_sEnterWay;

    //zhuhangmin 20181019  mutex_ 一把锁管理机器人的所有状态,目前没有需要细化数据颗粒锁的必要
    std::mutex		mutex_;
    int32_t			m_playerRoomStatus{0}; //default not in room
    bool		logon_{false}; // 是否登入大厅

};

using RobotPtr = std::shared_ptr<Robot>;