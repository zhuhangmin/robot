#include "stdafx.h"
#include "robot_utils.h"
#include "main.h"
#include "setting_manager.h"
#include "robot_hall_manager.h"
#include "robot_define.h"

SendMsgCountMap RobotUtils::send_msg_count_map;

int RobotUtils::SendRequestWithLock(const CDefSocketClientPtr& connection, const RequestID& requestid, const google::protobuf::Message &val, REQUEST& response, const bool& need_echo /*= true*/) {
    if (!connection) {
        ASSERT_FALSE_RETURN;
    }

    if (connection->IsConnected()) {
        ASSERT_FALSE_RETURN;
    }

    CHECK_REQUESTID(requestid);
    CONTEXT_HEAD	context_head = {};
    context_head.hSocket = connection->GetSocket();
    context_head.lSession = 0;
    context_head.bNeedEcho = need_echo;

    std::unique_ptr<char[]> data(new char[val.ByteSize()]);
    const auto is_succ = val.SerializePartialToArray(data.get(), val.ByteSize());
    if (!is_succ) {
        LOG_ERROR("SerializePartialToArray failed.");
        ASSERT_FALSE_RETURN;
    }

    REQUEST request(requestid, data.get(), val.ByteSize());

    BOOL timeout = false;
    const auto result = connection->SendRequest(&context_head, &request, &response, timeout, RequestTimeOut);
    send_msg_count_map[requestid] = send_msg_count_map[requestid] + 1;

    if (!result) {
        LOG_ERROR("send request fail");
        ASSERT_FALSE;
        if (timeout) {
            LOG_WARN("connection addr [%x] time out requestid = [%d]", connection.get(), requestid);
            return kConnectionTimeOut;
        }
        return kOperationFailed;
    }

    const auto responseid = response.head.nRequest;

    if (0 == responseid) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

CString RobotUtils::ExecHttpRequestPost(const CString& url, const CString& params) {
    if (!url) {
        assert(false);
        return "";
    }

    CString		result_str, server_str, object_str;
    CInternetSession* pSession = NULL;
    CHttpConnection*   pHttpConn = NULL;
    CHttpFile*   pHTTPFile = NULL;
    INTERNET_PORT   nPort;
    DWORD   dwServiceType;
    DWORD  retcode = -1;
    try {
        pSession = new CInternetSession();
        (void) pSession->SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, HttpTimeOut);	//重试之间的等待延时
        (void) pSession->SetOption(INTERNET_OPTION_CONNECT_RETRIES, 1);			//重试次数

        (void) AfxParseURL((LPCTSTR) url, dwServiceType, server_str, object_str, nPort);
        pHttpConn = pSession->GetHttpConnection(server_str, nPort);
        pHTTPFile = pHttpConn->OpenRequest(0, object_str, NULL, 1, NULL, "HTTP/1.1", INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_AUTO_REDIRECT);
        if (pHTTPFile) {
            (void) pHTTPFile->AddRequestHeaders("Content-Type:   application/json");
            (void) pHTTPFile->AddRequestHeaders("Accept:   */*");
            (void) pHTTPFile->SendRequest(NULL, 0, (LPTSTR) (LPCTSTR) params, params.GetLength());
            (void) pHTTPFile->QueryInfoStatusCode(retcode);

            CString   text;
            while (pHTTPFile->ReadString(text)) {
                result_str += text;
            }
        }
    } catch (...) {
        LOG_ERROR(_T("ExecHttpRequestPost catch: [%s] retcode: [%d] error: [%d]"), url, retcode, GetLastError());
        assert(false);
        return "";
    }

    if (pHTTPFile) { pHTTPFile->Close(); delete  pHTTPFile;  pHTTPFile = NULL; }
    if (pHttpConn) { pHttpConn->Close(); delete  pHttpConn;  pHttpConn = NULL; }
    if (pSession) { pSession->Close();  delete   pSession;   pSession = NULL; }

    if (result_str == "") {
        LOG_ERROR(_T("ExecHttpRequestPost urlPath: [%s] retcode: [%d] error: [%d]"), url, retcode, GetLastError());
        assert(false);
        return "";

    }
    return result_str;
}

int RobotUtils::GenRandInRange(const int& min_value, const int& max_value, int& random_result) {
    if (max_value < min_value) {
        LOG_ERROR("MAX VALUE %d smaller than SMALL VALUE %d", max_value, min_value);
        ASSERT_FALSE_RETURN;
    }
    static std::default_random_engine default_engine(std::time(nullptr));
    const auto raw_result = default_engine();
    random_result = raw_result % (max_value - min_value + 1) + min_value;
    return kCommSucc;
}

std::string RobotUtils::GetGameIP() {
    if (g_localGameIP.empty()) {
        TCHAR game_ip_str[MAX_PATH] = {0};
        GetPrivateProfileString("local_ip", "ip", LocalIPStr.c_str(), game_ip_str, MAX_PATH, g_szIniFile);
        g_localGameIP = std::string(game_ip_str);
    }
    return g_localGameIP;
}

int RobotUtils::GetGamePort() {
    auto room_setting_map = SettingMgr.GetRoomSettingMap();
    if (room_setting_map.empty()) {
        LOG_ERROR(_T("room_setting_map empty"));
        ASSERT_FALSE_RETURN;
    }

    auto room_id = InvalidRoomID;
    for (auto& kv: room_setting_map) {
        room_id = kv.first;
        break;
    }

    HallRoomData hall_room_data = {};
    if (kCommSucc != HallMgr.GetHallRoomData(room_id, hall_room_data)) {
        LOG_ERROR("GetHallRoomData room id  = [%d] failed", room_id);
        ASSERT_FALSE_RETURN;
    }

    return hall_room_data.room.nGamePort;
}

int RobotUtils::IsValidGameID(const GameID& game_id) {
    return game_id <= InvalidGameID ? kCommFaild : kCommSucc;
}

int RobotUtils::IsValidUserID(const UserID& userid) {
    return userid <=InvalidUserID ? kCommFaild : kCommSucc;
}

int RobotUtils::IsValidRoomID(const RoomID& roomid) {
    return roomid <= InvalidRoomID ? kCommFaild : kCommSucc;
}

int RobotUtils::IsValidTableNO(const TableNO& tableno) {
    return tableno <= InvalidTableNO ? kCommFaild : kCommSucc;
}

int RobotUtils::IsValidChairNO(const ChairNO& chairno) {
    return chairno <= InvalidChairNO ? kCommFaild : kCommSucc;
}

int RobotUtils::IsValidTokenID(const TokenID& tokenid) {
    return tokenid <= InvalidTokenID ? kCommFaild : kCommSucc;
}

int RobotUtils::IsValidRequestID(const RequestID& requestid) {
    return requestid <= InvalidRequestID ? kCommFaild : kCommSucc;
}

int RobotUtils::IsValidUser(const UserPtr& user) {
    return user == nullptr ? kCommFaild : kCommSucc;
}

int RobotUtils::IsValidTable(const TablePtr& table) {
    return table == nullptr ? kCommFaild : kCommSucc;
}

int RobotUtils::IsValidRoom(const RoomPtr& room) {
    return room == nullptr ? kCommFaild : kCommSucc;
}

int RobotUtils::IsValidRobot(const RobotPtr& robot) {
    return robot == nullptr ? kCommFaild : kCommSucc;
}

int RobotUtils::IsValidGameIP(const std::string& game_ip) {
    return game_ip.empty() ? kCommFaild : kCommSucc;
}

int RobotUtils::IsValidGamePort(const int32_t& game_port) {
    return game_port <= 0 ? kCommFaild : kCommSucc;
}

int RobotUtils::IsCurrentThread(YQThread& thread) {
    return  thread.GetThreadID() != GetCurrentThreadId() ? kCommFaild : kCommSucc;
}

int RobotUtils::NotThisThread(YQThread& thread) {
    return  thread.GetThreadID() == GetCurrentThreadId() ? kCommFaild : kCommSucc;
}


