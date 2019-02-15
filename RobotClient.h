#pragma once
#include "RobotDef.h"



class CRobotClient {
public:
    CRobotClient(const stRobotUnit& robot);
    virtual ~CRobotClient();

public:
    CCritSec*	Critical() { return &m_csClit; }
    int32_t		GetUserID() { return m_Account; }
    std::string	Password() { return m_Password; }
    int32_t		GameId() { return m_nGameId; }
    int32_t		GetRoomID() { return m_nRoomId; }
    bool		IsGaming() { return m_bRunGame; }
    void		SetGaming(bool isGame) { m_bRunGame = isGame; }
    TokenID	RoomToken() { return m_ConnRoom->GetTokenID(); }
    TokenID	GameToken() { return m_ConnGame->GetTokenID(); }

    void		SetLogonData(LPLOGON_SUCCEED_V2 logonOk) { m_LogonData = *logonOk; }

    bool        IsLogon() { return logon_; }

    void        SetLogon(bool status) { logon_ = status; }

    bool IsInRoom() { return m_nRoomId != 0; }
    void SetRoomID(RoomID roomid) { m_nRoomId = roomid; }


    void		OnDisconnRoom();
    void		OnDisconnGame();

    // for Room
    bool		ConnectRoom(const std::string& strIP, const int32_t nPort, uint32_t nThrdId);
    TTueRet		SendRoomRequest(TReqstId nReqId, uint32_t& nDataLen, void *pData, TReqstId &nRespId, void* &pRetData, bool bNeedEcho = true, uint32_t wait_ms = REQ_TIMEOUT_INTERVAL);

    // for Game
    bool		ConnectGame(const std::string& strIP, const int32_t nPort, uint32_t nThrdId);
    TTueRet		SendGameRequest(TReqstId nReqId, uint32_t& nDataLen, void *pData, TReqstId &nRespId, void* &pRetData, bool bNeedEcho = true, uint32_t wait_ms = REQ_TIMEOUT_INTERVAL);

    // for Send
    TTueRet		SendEnterRoom(const ROOM& room, uint32_t nNofifyThrId);
    TTueRet		SendEnterGame(const ROOM& room, uint32_t nNofifyThrId, std::string sNick, std::string sPortr, int nTableNo, int nChairNo);
    //@zhuhangmin 20181017
    TTueRet		SendGetNewTable(const ROOM& room, uint32_t nNofifyThrId, NTF_GET_NEWTABLE& lpNewTableInfo);


    TTueRet		SendRoomPulse();
    TTueRet		SendGamePulse();

    TTueRet		SendCheckVersion();

    int GetPlayerRoomStatus() {
        std::lock_guard<std::mutex> lg(m_mutex);
        return m_playerRoomStatus;
    }

    void SetPlayerRoomStatus(int status) {
        std::lock_guard<std::mutex> lg(m_mutex);
        m_playerRoomStatus = status;
        UWL_INF("SetPlayerRoomStatus = %d", status);
    }
protected:
    CCritSec        m_csClit;
    int32_t			m_Account{};
    std::string		m_Password;
    int32_t			m_nGameId{};
    int32_t			m_nRoomId{};
    bool			m_bRunGame{false};
    LOGON_SUCCEED_V2   m_LogonData{};
    ENTER_ROOM_OK   m_EnterRoomData{};
    PLAYER			m_PlayerData{};

    CCritSec        m_csConnRoom;
    CDefSocketClient* m_ConnRoom{new CDefSocketClient()};

    CCritSec        m_csConnGame;
    CDefSocketClient* m_ConnGame{new CDefSocketClient()};

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
    time_t			m_nNeedEnterG{0};
    int32_t			m_nToTable{0};
    int32_t			m_nToChair{0};
    std::string		m_sEnterWay;

    //zhuhangmin 20181019
    int32_t			m_playerRoomStatus{0}; //default not in room
    std::mutex		m_mutex;
    bool		logon_{false}; // 是否登入大厅

};