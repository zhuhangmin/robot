#include "stdafx.h"
#include "robot_utils.h"
#include "main.h"
#include "setting_manager.h"
#include "robot_hall_manager.h"
#include "robot_define.h"
#include "common_func.h"
#include "robot_statistic.h"
#include "PBReq.h"

int RobotUtils::SendRequestWithLock(const CDefSocketClientPtr& connection, const RequestID& requestid, const google::protobuf::Message &val, REQUEST& response, const bool& need_echo /*= true*/) {
    if (!connection) {
        LOG_WARN("connection not exist");
        return kCommFaild;
    }

    if (!connection->IsConnected()) {
        ASSERT_FALSE_RETURN;
    }

    CHECK_REQUESTID(requestid);
    CONTEXT_HEAD	context_head = {};
    context_head.hSocket = connection->GetSocket();
    context_head.lSession = 0;
    context_head.bNeedEcho = need_echo;

    std::unique_ptr<char[]> data(new char[val.ByteSize()]);
    const auto parse_result = val.SerializePartialToArray(data.get(), val.ByteSize());
    if (!parse_result) {
        LOG_ERROR("parse fail requestid [%d] [%s] content [%s] failed, result [%d]", requestid, REQ_STR(requestid), GetStringFromPb(val).c_str(), parse_result);
        ASSERT_FALSE_RETURN;
    }


    REQUEST request(requestid, data.get(), val.ByteSize());

    EVENT_TRACK(EventType::kSend, requestid);
    BOOL timeout = false;
    const auto result = connection->SendRequest(&context_head, &request, &response, timeout, RequestTimeOut);
    EVENT_TRACK(EventType::kRecv, requestid);

    if (!result) {

        if (timeout) {
            LOG_ERROR("[TIMEOUT] requestid [%d] [%s] content [%s] failed, result [%d]", requestid, REQ_STR(requestid), GetStringFromPb(val).c_str(), result);
            EVENT_TRACK(EventType::kErr, kConnectionTimeOut);
            ASSERT_FALSE;
            return kConnectionTimeOut;
        }
        EVENT_TRACK(EventType::kErr, kOperationFailed);
        ASSERT_FALSE;
        return kOperationFailed;
    }

    if (requestid != GR_RS_PULSE) {
        LOG_INFO("token [%d] [SEND] [%d] [%s]", connection->GetTokenID(), requestid, REQ_STR(requestid));
    }

    const auto responseid = response.head.nRequest;
    if (need_echo) {
        if (0 == responseid) {
            EVENT_TRACK(EventType::kErr, kRespIDZero);
            ASSERT_FALSE_RETURN;
        }
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

int RobotUtils::GenRandInRange(const int64_t& min_value, const int64_t& max_value, int64_t& random_result) {
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

int RobotUtils::IsValidConnection(const CDefSocketClientPtr& connection_) {
    return connection_ == nullptr ? kCommFaild : kCommSucc;
}

int RobotUtils::IsNegativeDepositAmount(const int64_t& deposit_amount) {
    return deposit_amount < 0 ? kCommFaild : kCommSucc;
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


int RobotUtils::TraceStack() {
    static const int MAX_STACK_FRAMES = 5;

    void *pStack[MAX_STACK_FRAMES];

    HANDLE process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);
    WORD frames = CaptureStackBackTrace(0, MAX_STACK_FRAMES, pStack, NULL);

    std::ostringstream oss;
    for (WORD i = 0; i < frames; ++i) {
        DWORD64 address = (DWORD64) (pStack[i]);

        DWORD64 displacementSym = 0;
        char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO pSymbol = (PSYMBOL_INFO) buffer;
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_SYM_NAME;

        DWORD displacementLine = 0;
        IMAGEHLP_LINE64 line;
        //SymSetOptions(SYMOPT_LOAD_LINES);
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        if (SymFromAddr(process, address, &displacementSym, pSymbol)
            && SymGetLineFromAddr64(process, address, &displacementLine, &line)) {
            oss << "\t" << pSymbol->Name << " at " << line.FileName << ":" << line.LineNumber << "(0x" << std::hex << pSymbol->Address << std::dec << ")" << std::endl;
        } else {
            oss << "\terror: " << GetLastError() << std::endl;
        }
    }

    LOG_WARN("[STACK] %s", oss.str().c_str());
    return kCommSucc;
}

std::string RobotUtils::ErrorCodeInfo(int code) {
    auto error_string = "???????????";
    switch (code) {
        case kCommFaild:
            error_string = "kCommFaild";
            break;
        case kInvalidParam:
            error_string = "kInvalidParam";
            break;
        case kInternalErr:
            error_string = "kInternalErr";
            break;

        case kInvalidUser:
            error_string = "kInvalidUser";
            break;
        case kInvalidRoomID:
            error_string = "kInvalidRoomID";
            break;
        case kAllocTableFaild:
            error_string = "kAllocTableFaild";
            break;
        case kUserNotFound:
            error_string = "kUserNotFound";
            break;
        case kTableNotFound:
            error_string = "kTableNotFound";
            break;
        case kTooMuchDeposit:
            error_string = "kTooMuchDeposit";
            break;
        case kTooLessDeposit:
            error_string = "kTooLessDeposit";
            break;
        case kHall_UserNotLogon:
            error_string = "kHall_UserNotLogon";
            break;
        case kHall_InvalidHardID:
            error_string = "kHall_InvalidHardID";
            break;
        case kHall_InOtherGame:
            error_string = "kHall_InOtherGame";
            break;
        case kHall_InvalidRoomID:
            error_string = "kHall_InvalidRoomID";
            break;

        case kCreateHallConnFailed:
            error_string = "kCreateHallConnFailed";
            break;
        case kCreateGameConnFailed:
            error_string = "kCreateGameConnFailed";
            break;
        case kConnectionTimeOut:
            error_string = "kConnectionTimeOut";
            break;
        case kOperationFailed:
            error_string = "kOperationFailed";
            break;
        case kRespIDZero:
            error_string = "kRespIDZero";
            break;
        case GR_HARDID_MISMATCH:
            error_string = "GR_HARDID_MISMATCH";
            break;
        default:
            LOG_WARN("UNKNOW ERROR CODE [%d]", code);
            break;
    }
    return error_string;
}

std::string RobotUtils::RequestStr(const RequestID& requestid) {
    std::string ret_string = "???????????";
    switch (requestid) {
        case GR_ENTER_NORMAL_GAME:
            ret_string = "GR_ENTER_NORMAL_GAME";
            break;
        case GR_ENTER_PRIVATE_GAME:
            ret_string = "GR_ENTER_PRIVATE_GAME";
            break;
        case GR_ENTER_MATCH_GAME:
            ret_string = "GR_ENTER_MATCH_GAME";
            break;
        case GR_LEAVE_GAME:
            ret_string = "GR_LEAVE_GAME";
            break;
        case GR_GIVE_UP:
            ret_string = "GR_GIVE_UP";
            break;
        case GS_START_GAME:
            ret_string = "GR_RS_START_GAME";
            break;
        case GR_RS_PULSE:
            ret_string = " GR_RS_PULSE";
            break;
        case GR_SWITCH_TABLE:
            ret_string = "GR_SWITCH_TABLE";
            break;
        case GR_TABLE_CHAT:
            ret_string = "GR_TABLE_CHAT";
            break;
        case GR_PLAYER2LOOKER:
            ret_string = "GR_PLAYER2LOOKER";
            break;
        case GR_LOOKER2PLAYER:
            ret_string = "GR_LOOKER2PLAYER";
            break;
        case GR_GET_TABLE_PLAYERS:
            ret_string = "GR_GET_TABLE_PLAYERS";
            break;
        case GR_MALL_SHOPING:
            ret_string = "GR_MALL_SHOPING";
            break;
        case GR_GET_PRODUCTS:
            ret_string = "GR_GET_PRODUCTS";
            break;
        case GN_TABLE_CHAT:
            ret_string = "GN_TABLE_CHAT";
            break;
        case GN_COUNTDOWN_START:
            ret_string = "GN_COUNTDOWN_START";
            break;
        case GN_COUNTDOWN_STOP:
            ret_string = "GN_COUNTDOWN_STOP";
            break;
        case GN_GAME_START:
            ret_string = "GN_GAME_START";
            break;
        case GN_PLAYER_GIVEUP:
            ret_string = "GN_PLAYER_GIVEUP";
            break;
        case GN_USER_SITDOWN:
            ret_string = "GN_USER_SITDOWN";
            break;
        case GN_USER_STANDUP:
            ret_string = "GN_USER_STANDUP";
            break;
        case GN_USER_LEAVE:
            ret_string = "GN_USER_LEAVE";
            break;
        case GR_RS_VALID_ROBOTSVR:
            ret_string = "GR_RS_VALID_ROBOTSVR";
            break;
        case GR_RS_GET_GAMEUSERS:
            ret_string = "GR_RS_GET_GAMEUSERS";
            break;
        case GN_RS_PLAER_ENTERGAME:
            ret_string = "GN_RS_PLAER_ENTERGAME";
            break;
        case GN_RS_LOOKER_ENTERGAME:
            ret_string = "GN_RS_LOOKER_ENTERGAME";
            break;
        case GN_RS_LOOER2PLAYER:
            ret_string = "GN_RS_LOOER2PLAYER";
            break;
        case GN_RS_PLAYER2LOOKER:
            ret_string = "GN_RS_PLAYER2LOOKER";
            break;
        case GN_RS_GAME_START:
            ret_string = "GN_RS_GAME_START";
            break;
        case GN_RS_USER_REFRESH_RESULT:
            ret_string = "GN_RS_USER_REFRESH_RESULT";
            break;
        case GN_RS_REFRESH_RESULT:
            ret_string = "GN_RS_REFRESH_RESULT";
            break;
        case GN_RS_USER_LEAVEGAME:
            ret_string = "GN_RS_USER_LEAVEGAME";
            break;
        case GN_RS_SWITCH_TABLE:
            ret_string = "GN_RS_SWITCH_TABLE";
            break;
        case GR_GET_ROOM:
            ret_string = "HALL GR_GET_ROOM";
            break;
        case GR_HALLUSER_PULSE:
            ret_string = "HALL GR_HALLUSER_PULSE";
            break;
        case GR_LOGON_USER_V2:
            ret_string = "HALL GR_LOGON_USER_V2";
            break;
        case UR_SOCKET_ERROR:
            ret_string = "UR_SOCKET_ERROR";
            break;
        case UR_SOCKET_CLOSE:
            ret_string = "UR_SOCKET_CLOSE";
            break;
        case PB_NOTIFY_TO_CLIENT:
            ret_string = "PB_NOTIFY_TO_CLIENT"; //zxd ： 不需要处理
            break;
        case MR_QUERY_USER_GAMEINFO:
            ret_string = "MR_QUERY_USER_GAMEINFO";
            break;
        default:
            LOG_WARN("UNKNOW REQUEST ID [%d]", requestid);
            break;
    }
    return ret_string;
}

std::string RobotUtils::UserTypeStr(const int& type) {
    std::string ret_string = "???????????";
    switch (type) {
        case kUserNormal:
            ret_string = "kUserNormal";
            break;
        case kUserAdmin:
            ret_string = "kUserAdmin";
            break;
        case kUserSuperAdmin:
            ret_string = "kUserSuperAdmin";
            break;
        case kUserRobot:
            ret_string = "kUserRobot";
            break;
        case USER_TYPE_HANDPHONE:
            ret_string = "USER_TYPE_HANDPHONE";
            break;
        case USER_TYPE_HANDPHONE |USER_TYPE_MERCHANT:
            ret_string = "USER_TYPE_HANDPHONE |USER_TYPE_MERCHANT";
            break;
        default:
            LOG_WARN("type [%x]", type);
            break;
    }

    return ret_string;
}

std::string RobotUtils::TableStatusStr(const int& status) {
    std::string ret_string = "???????????";
    switch (status) {
        case kTablePlaying:
            ret_string = "kTablePlaying";
            break;
        case kTableWaiting:
            ret_string = "kTableWaiting";
            break;
        default:
            break;
    }

    return ret_string;
}

std::string RobotUtils::ChairStatusStr(const int& status) {
    std::string ret_string = "???????????";
    switch (status) {
        case kChairPlaying:
            ret_string = "kChairPlaying";
            break;
        case kChairWaiting:
            ret_string = "kChairWaiting";
            break;
        default:
            assert(false);
            break;
    }

    return ret_string;
}

std::string RobotUtils::TimeStampToDate(const int& time_stamp) {
    if (time_stamp <= 0) {
        return "";
    }
    struct tm t = {0};
    time_t rawtime = time_stamp;
    time(&rawtime);
    localtime_s(&t, &rawtime);
    char str[256] = {0};
    asctime_s(str, sizeof str, &t);
    auto ret = std::string(str);
    const auto pos = ret.find('\n');
    if (pos != std::string::npos) {
        ret = ret.erase(pos);
    }
    return ret;

}
