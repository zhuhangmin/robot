#include "stdafx.h"
#include "app_delegate.h"
#include "main.h"
#include "robot_net_manager.h"
#include "setting_manager.h"
#include "game_net_manager.h"
#include "deposit_http_manager.h"
#include "robot_hall_manager.h"
#include "robot_utils.h"
#include "user_manager.h"
#include "room_manager.h"
#include "deposit_data_manager.h"

int AppDelegate::InitLanuch() const {
    LOG_INFO_FUNC("\t[START]");
    g_hExitServer = CreateEvent(NULL, TRUE, FALSE, NULL);

    TCHAR szFullName[MAX_PATH];
    GetModuleFileName(GetModuleHandle(NULL), szFullName, sizeof szFullName);
    UwlSplitPath(szFullName, SPLIT_DRIVE_DIR, g_szIniFile);
    g_curExePath = g_szIniFile;
    lstrcat(g_szIniFile, "RobotTool");
    lstrcat(g_szIniFile, _T(".ini"));

    TCHAR szPID[32];
    sprintf_s(szPID, _T("%d"), GetCurrentProcessId());
    WritePrivateProfileString(_T("listen"), _T("pid"), szPID, g_szIniFile);

    g_nClientID = GetPrivateProfileInt(_T("listen"), _T("clientid"), 0, g_szIniFile);
    if (0 == g_nClientID) {
        LOG_ERROR("\t[START] invalid clientid 0!");
        ASSERT_FALSE_RETURN;

    }
    LOG_INFO("\t[START] clientid = [%d]", g_nClientID);
    return kCommSucc;
}

int AppDelegate::Init() {
    LOG_INFO_FUNC("\t[START]");
    if (S_FALSE == CoInitialize(NULL))
        ASSERT_FALSE_RETURN;

    // ������ע�⡿ ���ɵ���˳����߲��г�ʼ�� ����
    // �����������ʼ����������ϵ����ʹ��ͬ��io��
    if (kCommSucc != InitLanuch()) {
        LOG_ERROR(_T("InitBase() return failed"));
        ASSERT_FALSE_RETURN;
    }

    // ����������
    if (kCommSucc != SettingMgr.Init()) {
        LOG_ERROR(_T("SettingManager Init Failed"));
        ASSERT_FALSE_RETURN;
    }

    // �����˲���������
    if (kCommSucc != DepositHttpMgr.Init()) {
        LOG_ERROR(_T("RobotDepositManager Init Failed"));
        ASSERT_FALSE_RETURN;
    }

    // �����˴���������
    if (kCommSucc != HallMgr.Init()) {
        LOG_ERROR(_T("RobotHallManager Init Failed"));
        ASSERT_FALSE_RETURN;
    }

    // ��Ϸ�������ݹ�����
    const auto game_port = RobotUtils::GetGamePort();
    const auto game_ip = RobotUtils::GetGameIP();
    if (kCommSucc != GameMgr.Init(game_ip, game_port)) {
        LOG_ERROR(_T("RobotGameInfoManager Init Failed"));
        ASSERT_FALSE_RETURN;
    }

    // ����ʱ �����л����˵�½����
    if (kCommSucc != ConnectHallForAllRobot()) {
        LOG_ERROR(_T("ConnectHallForAllRobot fail"));
        ASSERT_FALSE_RETURN;
    }

    // ��������Ϸ������
    if (kCommSucc != RobotMgr.Init()) {
        LOG_ERROR(_T("RobotGameManager Init Failed"));
        ASSERT_FALSE_RETURN;
    }

    // ����ʱ Ϊ������Ϸ�еĻ����� ������Ϸ����
    if (kCommSucc != ConnectGameForRobotInGame()) {
        LOG_ERROR(_T("ConnectGameForRobotInGame fail"));
        ASSERT_FALSE_RETURN;
    }

    // �������ݹ������Ϊ�̰߳�ȫ

    // ������ע�⡿����ʹ�ö��߳�ʵ�ֵ��ȣ���
    // ���ڡ�ThreadMainProc ���̡߳�ʵ�ֵ���
    // ��ҵ��������£����̹߳���ҵ�������߳̾�������
    timer_thread_.Initial(std::thread([this] {this->ThreadMainProc(); }));

    // ��ǰ����״̬
    SettingMgr.BriefInfo();
    RobotMgr.BriefInfo();
    HallMgr.BriefInfo();
    UserMgr.BriefInfo();
    DepositHttpMgr.SnapShotObjectStatus();

    // ��ʼ�����
    TCHAR hard_id[32];
    xyGetHardID(hard_id);
    LOG_INFO("hard_id [%s]", hard_id);
    g_inited = true;
    return kCommSucc;
}

int AppDelegate::Term() {
    SetEvent(g_hExitServer);

    g_inited = false;

    DepositHttpMgr.Term();
    RobotMgr.Term();
    HallMgr.Term();
    GameMgr.Term();
    SettingMgr.Term();

    timer_thread_.Release();

    if (g_hExitServer) { CloseHandle(g_hExitServer); g_hExitServer = NULL; }

    CoUninitialize();
    LOG_INFO_FUNC("[EXIT]");
    LOG_INFO("\n ===================================SERVER EXIT=================================== \n");
    return kCommSucc;

}

int AppDelegate::ThreadMainProc() {

    LOG_INFO("\t[START] main timer thread [%d] started", GetCurrentThreadId());

    while (true) {
        const auto dwRet = WaitForSingleObject(g_hExitServer, SettingMgr.GetMainsInterval());
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }
        if (WAIT_TIMEOUT == dwRet) {
            if (!g_inited) continue;
            if (!GameMgr.IsGameDataInited()) continue;

            // ҵ��������
            MainProcess();
        }
    }

    LOG_INFO("[EXIT] main timer thread [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}

int AppDelegate::MainProcess() {
    // �����Ҫƥ������˵��û�����
    UserMap need_user_map;
    if (kCommSucc != GetRoomNeedUserMap(need_user_map)) ASSERT_FALSE_RETURN;
    if (need_user_map.empty()) return kCommSucc;

    // ������ͬ��������������ָ����������
    for (auto& kv : need_user_map) {
        const auto userid = kv.first;
        const auto roomid = kv.second->get_room_id();
        const auto tableno = kv.second->get_table_no();

        int64_t max = 0;
        int64_t min = 0;
        RoomMgr.GetTableDepositRange(roomid, tableno, max, min);
        int64_t room_max = 0;
        int64_t room_min = 0;
        RoomMgr.GetRoomDepositRange(room_max, room_min);
        LOG_INFO("[DEPOSIT] user [%d] roomid [%d] tableno [%d] chairno [%d], room_min [%I64d] room_max [%I64d], table deposit min [%I64d] max [%I64d] need robot",
                 userid, roomid, tableno, kv.second->get_chair_no(), room_min, room_max, min, max);

        // ���ѡһ��û�н�����Ϸ�Ļ�����userid
        auto robot_userid = InvalidUserID;
        auto result = GetRandomQualifiedRobotUserID(roomid, tableno, robot_userid);
        if (kCommSucc != result) {
            if (kExceptionNoRobotDeposit != result) {
                continue;
            }
            if (kExceptionNoMoreRobot != result) {
                continue;
            }
            ASSERT_FALSE;
            continue;
        }
        if (InvalidUserID == robot_userid) {
            ASSERT_FALSE;
            continue;
        }

        // ���������� ����ָ����������
        result = RobotProcess(robot_userid, roomid, tableno);
        if (kCommSucc != result) {
            if (kTooLessDeposit == result || kTooMuchDeposit == result) {
                continue;
            }
            if (kAllocTableFaild == result) {
                return result;
            }
            if (kHall_InOtherGame == result) {
                return result;
            }
            ASSERT_FALSE_RETURN;
            continue;
        }
    }

    return kCommSucc;
}

int AppDelegate::RobotProcess(const UserID& userid, const RoomID& roomid, const TableNO& tableno) {
    LOG_ROUTE("beg", roomid, InvalidTableNO, userid);
    CHECK_USERID(userid);
    CHECK_ROOMID(roomid);
    CHECK_ROOMID(tableno);

    if (kCommSucc != LogonHall(userid)) {
        ASSERT_FALSE_RETURN;
    }

    const auto result = EnterGame(userid, roomid, tableno);
    if (kCommSucc != result) {
        if (kTooLessDeposit == result || kTooMuchDeposit == result) {
            return result;
        }
        if (kAllocTableFaild == result) {
            return result;
        }
        if (kHall_InOtherGame == result) {
            return result;
        }
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int AppDelegate::LogonHall(const UserID& userid) const {
    CHECK_USERID(userid);
    LOG_ROUTE("LogonHall", InvalidRoomID, InvalidTableNO, userid);

    if (kCommSucc != HallMgr.LogonHall(userid)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int AppDelegate::GetUserGameInfo(const UserID& userid) const {
    CHECK_USERID(userid);
    LOG_ROUTE("GetUserGameInfo", InvalidRoomID, InvalidTableNO, userid);

    if (kCommSucc != HallMgr.GetUserGameInfo(userid)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int AppDelegate::EnterGame(const UserID& userid, const RoomID& roomid, const TableNO& tableno) {
    CHECK_USERID(userid);
    CHECK_ROOMID(roomid);
    CHECK_TABLENO(tableno);

    LOG_ROUTE("enter game", roomid, tableno, userid);
    const auto result = RobotMgr.EnterGame(userid, roomid, tableno);

    // ����Ϸʧ�ܴ�����
    if (kCommSucc !=result) {
        LOG_ERROR("SendEnterGame resp error result [%d] str [%s], roomid [%d] userid [%d]", result, ERR_STR(result), roomid, userid);
        if (kTooLessDeposit == result) {
            return kTooLessDeposit;

        } else if (kTooMuchDeposit == result) {
            return kTooMuchDeposit;

        } else if (kHall_InOtherGame == result) {
            LOG_WARN("userid [%d] kHall_InOtherGame, add to filter list", userid);
            robot_in_other_game_map_[userid] = userid;

        } else if (kHall_UserNotLogon == result) {
            HallMgr.SetLogonStatus(userid, HallLogonStatusType::kNotLogon);

        } else if (result == kInvalidUser) { // �û����Ϸ���token��entergameʱ�����token��һ�£�
            LOG_ERROR("token dismatch with enter game token");

        } else if (result == kInvalidRoomID) {

        } else if (result == kAllocTableFaild) {

        } else if (result == kUserNotFound) {

        } else if (result == kTableNotFound) {

        } else if (result == kHall_InvalidHardID) {

        } else if (result == kHall_InvalidRoomID) {

        } else {
            LOG_WARN("unknow error code [%d][%s]", result, ERR_STR(result))
        }

        LOG_WARN("!!! ENTER GAME FAILED !!! [%d] tableno [%d] userid [%d] error code [%d][%s]", roomid, tableno, userid, result, ERR_STR(result));
        return result;
    }
    LOG_INFO("*** ENTER GAME SUCCESS*** roomid [%d] tableno [%d] userid [%d]", roomid, tableno, userid);
    return kCommSucc;
}

int AppDelegate::GetRandomQualifiedRobotUserID(const RoomID& roomid, const TableNO& tableno, UserID& random_userid) const {
    // ���� ������Ϸ�Ļ�����
    // ����Ϸ�Ļ����� �������Ӳ��� ����������Ϸ������� ҵ���������
    RobotUserIDMap not_logon_game_temp;
    auto robot_setting = SettingMgr.GetRobotSettingMap();
    for (auto& kv : robot_setting) {
        auto userid = kv.first;
        if (UserMgr.IsRobotUserExist(userid)) {
            LOG_DEBUG("robot userid [%d] in game", userid);
            continue;
        }
        not_logon_game_temp[userid] = userid;
    }

    // ����Ϸ�еĻ����� 


    if (not_logon_game_temp.empty()) {
        LOG_WARN("no more robot in setting pool!");
        return kExceptionNoMoreRobot;
    }

    // ������������Ϸ���쳣������
    for (const auto& kv : robot_in_other_game_map_) {
        const auto userid = kv.first;
        if (not_logon_game_temp.find(userid) != not_logon_game_temp.end()) {
            LOG_DEBUG("filter robot in other game userid [%d]", userid);
            not_logon_game_temp.erase(userid);
        }
    }

    if (not_logon_game_temp.empty()) {
        LOG_WARN("no more quailfied robot in this game, some are in other game!");
        return kExceptionNoMoreRobot;
    }

    // �������ڲ���HTTP����Ļ�����
    const auto robot_in_deposit_process_map = DepositHttpMgr.GetDepositMap();
    for (const auto& kv : robot_in_deposit_process_map) {
        const auto userid = kv.first;
        if (not_logon_game_temp.find(userid) != not_logon_game_temp.end()) {
            LOG_DEBUG("filter robot in http deposit process userid [%d]", userid);
            not_logon_game_temp.erase(userid);
        }
    }

    if (not_logon_game_temp.empty()) {
        LOG_WARN("no more quailfied robot , some are in http deposit process!");
        return kExceptionNoMoreRobot;
    }

    // �������ڸ���������Ϣ�Ļ�����
    const auto update_deposit_map = HallMgr.GetUpdateDepositMap();
    for (const auto& kv : update_deposit_map) {
        const auto userid = kv.first;
        if (not_logon_game_temp.find(userid) != not_logon_game_temp.end()) {
            LOG_DEBUG("filter robot in hall deposit update process userid [%d]", userid);
            not_logon_game_temp.erase(userid);
        }
    }
    if (not_logon_game_temp.empty()) {
        LOG_WARN("no more quailfied robot , some are in hall deposit process!");
        return kExceptionNoMoreRobot;
    }

    // ���˷��������Ļ�����
    auto result = FilterRobotNotInRoomDepositRange(not_logon_game_temp);
    if (kCommSucc != result) {
        if (kExceptionNoRobotDeposit == result) {
            LOG_WARN("filter robot kExceptionNoRobotDeposit ");
            return result;
        }
        if (kExceptionGameDataNotInited == result) {
            LOG_WARN("filter robot kExceptionGameDataNotInited ");
            return result;
        }
        if (kExceptionRobotNotInGame == result) {
            LOG_WARN("filter robot exception robot not in game ");
            return result;
        }
        ASSERT_FALSE_RETURN;
    }

    if (not_logon_game_temp.empty()) {
        LOG_WARN("no more quailfied robot , some are not in room deposit range!");
        return kExceptionNoMoreRobot;
    }

    // �������������Ļ�����
    result = FilterRobotNotInTableDepositRange(roomid, tableno, not_logon_game_temp);
    if (kCommSucc != result) {
        if (kExceptionNoRobotDeposit == result) {
            LOG_WARN("[DEPOSIT] filter robot kExceptionNoRobotDeposit ");
            return result;
        }
        if (kExceptionGameDataNotInited == result) {
            LOG_WARN("[DEPOSIT] filter robot kExceptionGameDataNotInited ");
            return result;
        }
        if (kExceptionRobotNotInGame == result) {
            LOG_WARN("filter robot exception robot not in game ");
            return result;
        }
        ASSERT_FALSE_RETURN;
    }

    if (not_logon_game_temp.empty()) {
        LOG_WARN("no more quailfied robot , some are not in table deposit range!");
        return kExceptionNoMoreRobot;
    }

    // ���ѡȡuserid
    int64_t random_pos = 0;
    if (kCommSucc != RobotUtils::GenRandInRange(0, not_logon_game_temp.size() - 1, random_pos)) {
        ASSERT_FALSE_RETURN;
    }

    const auto random_it = std::next(std::begin(not_logon_game_temp), random_pos);
    random_userid = random_it->first;

    return kCommSucc;
}

int AppDelegate::GetRoomNeedUserMap(UserMap& need_user_map) {
    const int now = std::time(nullptr);
    const auto room_setting_map = SettingMgr.GetRoomSettingMap();

    UserMap filter_user_map;
    if (kCommSucc != GetWaittingUser(filter_user_map)) {
        ASSERT_FALSE_RETURN;
    }

    if (filter_user_map.empty()) {
        return kCommSucc;
    }

    // ���������ж��Ƿ���Ҫ�ɻ�����
    for (auto& kv_setting: room_setting_map) {
        const auto roomid = kv_setting.first;
        const auto setting = kv_setting.second;
        const auto wait_time = setting.wait_time; // seconds
        const auto count_per_table = setting.count_per_table;

        for (const auto& kv_user : filter_user_map) {
            const auto userid = kv_user.first;
            const auto user = kv_user.second;
            const auto tableno = user->get_table_no();

            // �ж����ϻ������Ƿ�ﵽ���������� 
            auto robot_count_on_chair = 0;
            if (kCommSucc != RoomMgr.GetRobotCountOnChairs(roomid, tableno, robot_count_on_chair)) {
                ASSERT_FALSE;
                continue;
            }

            if (robot_count_on_chair >= count_per_table) {
                //LOG_INFO("in count down process reach robot count roomid [%d], tableno [%d], already [%d], design [%d]",
                //roomid, tableno, robot_count_on_chair, count_per_table);
                continue;
            }

            // �ж���ҵȴ�ʱ�䣬�������õȴ���ֵ
            ChairInfo info;
            const auto result = RoomMgr.GetChairInfo(roomid, tableno, userid, info);
            if (kCommSucc != result) {
                if (kExceptionUserNotOnChair == result) {
                    continue;
                }
                ASSERT_FALSE;
                continue;
            }

            const int bind = info.get_bind_timestamp();
            if (bind <= 0) {
                assert(false);
                LOG_WARN("userid [%d] tableno [%d] roomid [%d] bind 0", userid, tableno, roomid);
                ASSERT_FALSE;
                continue;
            }

            const int elapse = now - bind;
            if (elapse >= wait_time) {
                LOG_INFO("userid [%d] now [%d] bind [%d] elapse [%d]", userid, now, bind, elapse);
                need_user_map[userid] = user;
            }
        }
    }

    return kCommSucc;
}

int AppDelegate::GetWaittingUser(UserMap& filter_user_map) const {
    // ��ȡ��ʵ��Ҽ��� 
    UserMap normal_user_map;
    if (kCommSucc != UserMgr.GetNormalUserMap(normal_user_map)) {
        ASSERT_FALSE_RETURN;
    }
    if (normal_user_map.empty()) return kCommSucc;

    for (const auto& kv : normal_user_map) {
        const auto userid = kv.first;
        const auto user = kv.second;
        const auto tableno = user->get_table_no();
        const auto roomid = user->get_room_id();

        if (0 == tableno || 0 == roomid) {
            //LOG_WARN("userid [%d] tableno [%d] roomid [%d]", userid, tableno, roomid);
            continue;
        }

        // �����ѿ���
        auto is_playing = true;
        if (kCommSucc != RoomMgr.IsTablePlaying(roomid, tableno, is_playing)) {
            continue;
        }
        if (is_playing) continue;

        // ���˷ǵȴ����
        ChairInfo info;
        const auto result = RoomMgr.GetChairInfo(roomid, tableno, userid, info);
        if (kCommSucc != result) {
            if (kExceptionUserNotOnChair == result) {
                continue;
            }
            ASSERT_FALSE;
            continue;
        }

        if (kChairWaiting != info.get_chair_status()) {
            continue;
        }

        // �������Ѿ��ﵽ����С��������
        auto normal_count_on_chair = 0;
        if (kCommSucc != RoomMgr.GetNormalCountOnChairs(roomid, tableno, normal_count_on_chair)) {
            ASSERT_FALSE;
            continue;
        }

        auto min_playercount_per_table = 0;
        if (kCommSucc != RoomMgr.GetMiniPlayers(roomid, min_playercount_per_table)) {
            ASSERT_FALSE;
            continue;
        }

        if (normal_count_on_chair >= min_playercount_per_table) {
            //LOG_INFO("reach  roomid[%d], tableno[%d], normal_count_on_chair[%d], min_playercount_per_table[%d]",
            //roomid, tableno, normal_count_on_chair, min_playercount_per_table);
            continue;
        }

        filter_user_map[userid] = user;
    }
    return kCommSucc;
}

int AppDelegate::ConnectHallForAllRobot() {
    LOG_INFO_FUNC("\t[START]");
    const auto robot_map = SettingMgr.GetRobotSettingMap();
    for (auto& kv : robot_map) {
        auto userid = kv.first;
        if (kCommSucc != RobotUtils::IsValidUserID(userid)) {
            ASSERT_FALSE;
            continue;
        }

#ifndef _DEBUG
        LOG_WARN("��ʽ���ֹ������Ϣ�����˷����� [������] ��ʽ���������������");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
#endif

        // ��½
        if (kCommSucc != LogonHall(userid)) {
            LOG_ERROR("userid [userid] ConnectHallForAllRobot failed", userid);
            ASSERT_FALSE;
            continue;
        }

        // ����û���Ϣ
        if (kCommSucc != GetUserGameInfo(userid)) {
            LOG_ERROR("userid [userid] ConnectHallForAllRobot failed", userid);
            ASSERT_FALSE;
            continue;
        }
    }
    return kCommSucc;
}

int AppDelegate::ConnectGameForRobotInGame() {
    LOG_INFO_FUNC("\t[START]");
    const auto users = UserMgr.GetAllUsers();
    for (auto& kv : users) {
        auto userid = kv.first;
        if (kCommSucc != RobotUtils::IsValidUserID(userid)) {
            ASSERT_FALSE;
            continue;
        }

        auto user = kv.second;
        if (kCommSucc != RobotUtils::IsValidUser(user)) {
            ASSERT_FALSE;
            continue;
        }

        const auto user_tpye = user->get_user_type();
        if (!IS_BIT_SET(user_tpye, kUserRobot)) {
            continue;
        }

        auto roomid = user->get_room_id();
        if (kCommSucc != RobotUtils::IsValidRoomID(roomid)) {
            ASSERT_FALSE;
            continue;
        }

        auto tableno = user->get_table_no();
        if (kCommSucc != RobotUtils::IsValidTableNO(tableno)) {
            ASSERT_FALSE;
            continue;
        }

        const auto result = EnterGame(userid, roomid, tableno);
        if (kCommSucc != result) {
            LOG_ERROR("userid [userid] tableno [%d] roomid [%d]  ConnectGameForRobotInGame failed", userid, tableno, roomid);
            if (kTooLessDeposit == result || kTooMuchDeposit == result) {
                continue;
            }
            ASSERT_FALSE;
            continue;
        }
    }
    return kCommSucc;
}

int AppDelegate::FilterRobotNotInRoomDepositRange(RobotUserIDMap& not_logon_game_temp) const {
    if (not_logon_game_temp.empty()) return kCommFaild;
    int64_t max = 0;
    int64_t min = 0;
    if (kCommSucc != RoomMgr.GetRoomDepositRange(max, min)) {
        LOG_WARN("room deposit range min [%I64d] max [%I64d]", min, max);
        return kCommFaild;
    }
    /*LOG_INFO("check robot deposit room deposit range min [%I64d] max [%I64d]", min, max);*/
    auto user_game_info_map = DepositDataMgr.GetUserGameDataMap();
    for (const auto& kv: user_game_info_map) {
        const auto userid = kv.first;
        const auto deposit = kv.second.nDeposit;
        if (deposit < 0) ASSERT_FALSE_RETURN;

        auto exist = false;
        if (kCommSucc != SettingMgr.IsRobotSettingExist(userid, exist)) {
            continue;
        }
        if (!exist) continue;

        if (deposit < min) {
            not_logon_game_temp.erase(userid);
            const auto amount = min - deposit + 1;
            LOG_INFO("[DEPOSIT] ROOM robot userid [%d] deposit [%I64d] amount [%I64d]", userid, deposit, amount);
            if (amount < 0) { ASSERT_FALSE; continue; }
            DepositHttpMgr.SetDepositTypeAmount(userid, DepositType::kGain, amount);
            continue;
        }

        if (deposit > max) {
            not_logon_game_temp.erase(userid);
            const auto amount = deposit - max + 1;
            LOG_INFO("[DEPOSIT] ROOM robot userid [%d] deposit [%I64d] amount [%I64d]", userid, deposit, amount);
            if (amount < 0) { ASSERT_FALSE; continue; }
            DepositHttpMgr.SetDepositTypeAmount(userid, DepositType::kBack, amount);
            continue;
        }
    }

    return kCommSucc;
}

int AppDelegate::FilterRobotNotInTableDepositRange(const RoomID& roomid, const TableNO& tableno, RobotUserIDMap& not_logon_game_temp) const {
    if (not_logon_game_temp.empty()) return kCommFaild;

    int64_t max = 0;
    int64_t min = 0;
    if (kCommSucc != RoomMgr.GetTableDepositRange(roomid, tableno, max, min)) {
        ASSERT_FALSE_RETURN;
    }
    if (max < min) ASSERT_FALSE_RETURN;
    if (max <= 0) ASSERT_FALSE_RETURN;
    if (min < 0) ASSERT_FALSE_RETURN;

    RobotDepositMap too_less_map;
    RobotDepositMap too_much_map;
    auto game_user_info_map = DepositDataMgr.GetUserGameDataMap();
    for (const auto& kv : game_user_info_map) {
        const auto userid = kv.first;
        const auto user_info = kv.second;
        const auto deposit = user_info.nDeposit;
        if (deposit < min) {
            not_logon_game_temp.erase(userid);
            too_less_map[userid] = deposit;
        }
        if (deposit > max) {
            not_logon_game_temp.erase(userid);
            too_much_map[userid] = deposit;
        }
    }

    if (not_logon_game_temp.empty()) {
        LOG_WARN("[DEPOSIT] no robot match the roomid [%d] tableno [%d] deposit range min [%I64d] max [%I64d] !", roomid, tableno, min, max);
        // ���ѡ��1�������˽���
        // ����
        if (too_less_map.size() > 0) {
            int64_t random_pos = 0;
            if (kCommSucc != RobotUtils::GenRandInRange(0, too_less_map.size() - 1, random_pos)) {
                ASSERT_FALSE_RETURN;
            }
            const auto random_it = std::next(std::begin(too_less_map), random_pos);
            const auto userid = random_it->first;
            const auto deposit = random_it->second;

            // ����Ϸ�еĻ����� �������Ӳ��� ����������Ϸ������� ҵ���������
            if (UserMgr.IsRobotUserExist(userid)) return kExceptionRobotNotInGame;

            int64_t random_diff = 0;
            if (kCommSucc != RobotUtils::GenRandInRange(min, max*0.7, random_diff)) {
                ASSERT_FALSE_RETURN;
            }
            if (deposit < min && deposit > 0 && random_diff > 0) {
                int64_t amount = random_diff - deposit;
                if (amount < 0) {
                    assert(false);
                    return kCommFaild;
                }
                LOG_INFO("[DEPOSIT] TABLE userid [%d] deposit [%I64d] random [%I64d] amount [%I64d]",
                         userid, deposit, random_diff, amount);
                DepositHttpMgr.SetDepositTypeAmount(userid, DepositType::kGain, amount);
            } else {
                ASSERT_FALSE_RETURN;
            }
        }

        // ����
        if (too_much_map.size() > 0) {
            int64_t amount = 0;
            int64_t random_pos = 0;
            if (kCommSucc != RobotUtils::GenRandInRange(0, too_much_map.size() - 1, random_pos)) {
                ASSERT_FALSE_RETURN;
            }
            const auto random_it = std::next(std::begin(too_much_map), random_pos);
            const auto userid = random_it->first;
            const auto deposit = random_it->second;

            // ����Ϸ�еĻ����� �������Ӳ��� ����������Ϸ������� ҵ���������
            if (UserMgr.IsRobotUserExist(userid)) return kExceptionRobotNotInGame;

            int64_t random_diff = 0;
            if (kCommSucc != RobotUtils::GenRandInRange(min, max*0.7, random_diff)) {
                ASSERT_FALSE_RETURN;
            }
            if (deposit > max && deposit > 0 && random_diff > 0) {
                int64_t amount = deposit - random_diff;
                if (amount < 0) {
                    assert(false);
                    return kCommFaild;
                }
                LOG_INFO("[DEPOSIT] TABLE robot userid [%d] deposit [%I64d] random [%I64d] amount [%I64d]", userid, deposit, random_diff, amount);
                DepositHttpMgr.SetDepositTypeAmount(userid, DepositType::kBack, amount);
            } else {
                ASSERT_FALSE_RETURN;
            }
        }
        return kExceptionNoRobotDeposit;
    }

    return kCommSucc;
}

#ifdef _DEBUG
// TEST ONLY
void AppDelegate::SendTestRobot(const UserID& userid, const RoomID& roomid, const TableNO& tableno) {
    // ���������� ����ָ����������
    if (kCommSucc != RobotProcess(userid, roomid, tableno)) {
        ASSERT_FALSE;
    }
}

int AppDelegate::TestLogonHall(const UserID& userid) const {
    CHECK_USERID(userid);
    LOG_ROUTE("LogonHall", InvalidRoomID, InvalidTableNO, userid);

    if (kCommSucc != HallMgr.LogonHall(userid)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}
#endif