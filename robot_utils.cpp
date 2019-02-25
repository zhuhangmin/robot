#include "stdafx.h"
#include "robot_utils.h"
#include "Main.h"
#include "setting_manager.h"
#include "robot_hall_manager.h"
#include "robot_define.h"


int RobotUtils::SendRequestWithLock(CDefSocketClientPtr& connection, RequestID requestid, const google::protobuf::Message &val, REQUEST& response, bool need_echo /*= true*/) {
    CONTEXT_HEAD	context_head = {};
    context_head.hSocket = connection->GetSocket();
    context_head.lSession = 0;
    context_head.bNeedEcho = need_echo;

    std::unique_ptr<char[]> data(new char[val.ByteSize()]);
    bool is_succ = val.SerializePartialToArray(data.get(), val.ByteSize());
    if (false == is_succ) {
        UWL_ERR("SerializePartialToArray failed.");
        return kCommFaild;
    }

    REQUEST request(requestid, data.get(), val.ByteSize());

    BOOL timeout = false;
    BOOL result = connection->SendRequest(&context_head, &request, &response, timeout, RequestTimeOut);

    if (!result)///if timeout or disconnect 
    {
        UWL_ERR("send request fail");
        //assert(false);
        return timeout ? ERROR_CODE::kConnectionDisable : ERROR_CODE::kOperationFailed;
    }

    auto responseid = response.head.nRequest;

    if (0 == responseid) {
        assert(false);
        /*       UWL_ERR("game respId = 0");
        CHAR info[512] = {};
        sprintf_s(info, "%s", pRetData.get());*/
        return kCommFaild;
    }
    return kCommSucc;
}

CString RobotUtils::ExecHttpRequestPost(const CString& url, const CString& params) {
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
        UwlTrace(_T("ExecHttpRequestPost catch:%s retcode:%d error: %d"), url, retcode, GetLastError());
        UwlLogFile(_T("ExecHttpRequestPost catch:%s retcode:%d error: %d"), url, retcode, GetLastError());
        assert(false);
    };

    if (pHTTPFile) { pHTTPFile->Close(); delete  pHTTPFile;  pHTTPFile = NULL; }
    if (pHttpConn) { pHttpConn->Close(); delete  pHttpConn;  pHttpConn = NULL; }
    if (pSession) { pSession->Close();  delete   pSession;   pSession = NULL; }

    if (result_str == "") {
        UwlTrace(_T("ExecHttpRequestPost urlPath:%s retcode:%d error: %d"), url, retcode, GetLastError());
        UwlLogFile(_T("ExecHttpRequestPost urlPath:%s retcode:%d error: %d"), url, retcode, GetLastError());
        assert(false);

    } else {
        UwlLogFile(_T("ExecHttpRequestPost strResult:%s "), result_str);
    }
    return result_str;
}

int RobotUtils::GenRandInRange(int min_value, int max_value, int& random_result) {
    if (max_value < min_value) {
        UWL_ERR("MAX VALUE %d smaller than SMALL VALUE %d", max_value, min_value);
        assert(0);
        return kCommFaild;
    }
    static std::default_random_engine default_engine(std::time(nullptr));
    auto raw_result = default_engine();
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
    if (room_setting_map.size() == 0) {
        UWL_ERR(_T("room_setting_map empty"));
        assert(false);
        return kCommFaild;
    }

    auto room_id = InvalidRoomID;
    for (auto& kv: room_setting_map) {
        room_id = kv.first;
        break;
    }

    HallRoomData hall_room_data;
    if (kCommFaild == HallMgr.GetHallRoomData(room_id, hall_room_data)) {
        UWL_ERR("GetHallRoomData room id = %d failed", room_id);
        assert(false);
        return kCommFaild;
    }

    return hall_room_data.room.nPort;
}

int RobotUtils::IsValidGameID(GameID game_id) {
    return game_id <= InvalidGameID ? kCommFaild : kCommSucc;
}

int RobotUtils::IsValidUserID(UserID userid) {
    return userid <=InvalidUserID ? kCommFaild : kCommSucc;
}

int RobotUtils::IsValidRoomID(RoomID roomid) {
    return roomid <= InvalidRoomID ? kCommFaild : kCommSucc;
}

int RobotUtils::IsValidTableNO(TableNO tableno) {
    return tableno <= InvalidTableNO ? kCommFaild : kCommSucc;
}

int RobotUtils::IsValidChairNO(ChairNO chairno) {
    return chairno <= InvalidChairNO ? kCommFaild : kCommSucc;
}

