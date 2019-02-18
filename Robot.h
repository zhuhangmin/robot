#pragma once
#include "RobotDef.h"



class Robot {
public:
    Robot(const RobotSetting& robot);
    virtual ~Robot();

public:
    int32_t		GetUserID() { return userid_; }
    std::string	Password() { return m_Password; }
    int32_t		GetRoomID() { return m_nRoomId; }
    bool		IsGaming() { return m_bRunGame; }
    void		SetGaming(bool isGame) { m_bRunGame = isGame; }
    TokenID	RoomToken() { return connection_room_->GetTokenID(); }
    TokenID	GameToken() { return connection_game_->GetTokenID(); }

    void		SetLogonData(LPLOGON_SUCCEED_V2 logonOk) { m_LogonData = *logonOk; }

    bool        IsLogon() { return logon_; }

    void        SetLogon(bool status) { logon_ = status; }

    bool IsInRoom() { return m_nRoomId != 0; }
    void SetRoomID(RoomID roomid) { m_nRoomId = roomid; }


    void		OnDisconnRoom();
    void		OnDisconnGame();

    // for Room


    // for Game



    // for Send
    TTueRet		SendEnterRoom(const ROOM& room, uint32_t nNofifyThrId);
    TTueRet		SendEnterGame(const ROOM& room, uint32_t nNofifyThrId, std::string sNick, std::string sPortr, int nTableNo, int nChairNo);
    //@zhuhangmin 20181017
    TTueRet		SendGetNewTable(const ROOM& room, uint32_t nNofifyThrId, NTF_GET_NEWTABLE& lpNewTableInfo);


    TTueRet		SendRoomPulse();
    TTueRet		SendGamePulse();

    int GetPlayerRoomStatus();

    void SetPlayerRoomStatus(int status);
private:
    bool ConnectRoomWithLock(const std::string& strIP, const int32_t nPort, uint32_t nThrdId); //caller must require with lock
    bool ConnectGameWithLock(const std::string& strIP, const int32_t nPort, uint32_t nThrdId);
    TTueRet SendRoomRequestWithLock(TReqstId nReqId, uint32_t& nDataLen, void *pData, TReqstId &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho = true, uint32_t wait_ms = REQ_TIMEOUT_INTERVAL);
    TTueRet SendGameRequestWithLock(TReqstId nReqId, uint32_t& nDataLen, void *pData, TReqstId &nRespId, std::shared_ptr<void> &pRetData, bool bNeedEcho = true, uint32_t wait_ms = REQ_TIMEOUT_INTERVAL);


protected:
    int32_t			userid_{0};
    std::string		m_Password;
    int32_t			m_nRoomId{};
    bool			m_bRunGame{false};
    LOGON_SUCCEED_V2   m_LogonData{};
    ENTER_ROOM_OK   m_EnterRoomData{};
    PLAYER			m_PlayerData{};

    CDefSocketClientPtr connection_room_{std::make_shared<CDefSocketClient>()};

    CDefSocketClientPtr connection_game_{std::make_shared<CDefSocketClient>()};

public:
    // ��Ϸ������
    int32_t			m_nWantRoomId{};

    // �Ƿ�Ҫȡ��
    bool			m_bGainDeposit{false};
    int64_t			m_nGainAmount{};

    // �Ƿ�Ҫ����
    bool			m_bBackDeposit{false};
    int32_t			m_nBackAmount{};

    // �ӳ�Enter
    int32_t			m_nToTable{0};
    int32_t			m_nToChair{0};
    std::string		m_sEnterWay;

    //zhuhangmin 20181019  mutex_ һ������������˵�����״̬,Ŀǰû����Ҫϸ�����ݿ������ı�Ҫ
    std::mutex		mutex_;
    int32_t			m_playerRoomStatus{0}; //default not in room
    bool		logon_{false}; // �Ƿ�������

};

using RobotPtr = std::shared_ptr<Robot>;