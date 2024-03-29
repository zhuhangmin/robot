#include "stdafx.h"
#include "app_delegate.h"
#include "main.h"
#include "robot_net_manager.h"
#include "setting_manager.h"
#include "game_net_manager.h"
#include "deposit_http_manager.h"
#include "robot_hall_manager.h"
#include "robot_utils.h"
//!!警告 请勿include 任何 data_manager 和 data 头文件
//!!会引起死锁问题
//!!只使用io_manager对象 setting 和 robotutils
#include "deposit_data_manager.h"

int AppDelegate::InitLanuch() const {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    LOG_INFO_FUNC("\t[START]");
    g_hExitServer = CreateEvent(NULL, TRUE, FALSE, NULL);

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
    ThreadID launch_thread_id = GetCurrentThreadId();
    CHECK_THREADID(launch_thread_id);
    g_launchThreadID = launch_thread_id;
    CHECK_MAIN_OR_LAUNCH_THREAD();
    LOG_INFO_FUNC("\t[START]");
    if (S_FALSE == CoInitialize(NULL))
        ASSERT_FALSE_RETURN;

    // ！！【注意】 不可调换顺序或者并行初始化 ！！
    // 以下数据类初始化有依赖关系，故使用同步io，
    if (kCommSucc != InitLanuch()) {
        LOG_ERROR(_T("InitBase() return failed"));
        ASSERT_FALSE_RETURN;
    }

    // 配置数据类
    if (kCommSucc != SettingConfig.Init()) {
        LOG_ERROR(_T("SettingManager Init Failed"));
        ASSERT_FALSE_RETURN;
    }

    // 机器人补银管理类
    if (kCommSucc != DepositHttpMgr.Init()) {
        LOG_ERROR(_T("RobotDepositManager Init Failed"));
        ASSERT_FALSE_RETURN;
    }

    // 机器人大厅管理类
    if (kCommSucc != HallMgr.Init()) {
        LOG_ERROR(_T("RobotHallManager Init Failed"));
        ASSERT_FALSE_RETURN;
    }

    // 游戏服务数据管理类
    const auto game_port = HallMgr.GetGamePort();
    const auto game_ip = RobotUtils::GetGameIP();
    if (kCommSucc != GameMgr.Init(game_ip, game_port)) {
        LOG_ERROR(_T("RobotGameInfoManager Init Failed"));
        ASSERT_FALSE_RETURN;
    }

    // 启动时 让所有机器人登陆大厅
    if (kCommSucc != ConnectHallForAllRobot()) {
        LOG_ERROR(_T("ConnectHallForAllRobot fail"));
        ASSERT_FALSE_RETURN;
    }

    // 机器人游戏管理类
    if (kCommSucc != RobotMgr.Init()) {
        LOG_ERROR(_T("RobotGameManager Init Failed"));
        ASSERT_FALSE_RETURN;
    }

    // 启动时 为已在游戏中的机器人 建立游戏连接
    if (kCommSucc != ConnectGameForRobotInGame()) {
        LOG_ERROR(_T("ConnectGameForRobotInGame fail"));
        ASSERT_FALSE_RETURN;
    }

    // 以上数据管理类均为线程安全

    // ！！【注意】请勿使用多线程实现调度！！
    // 请在【ThreadMainProc 单线程】实现调度
    // 无业务锁情况下，多线程构建业务会出现线程竞争问题
    timer_thread_.Initial(std::thread([this] {this->ThreadMainProc(); }));
    ThreadID main_thread_id = timer_thread_.GetThreadID();
    CHECK_THREADID(main_thread_id);
    g_mainThreadID = main_thread_id;

    // 当前数据状态
    SettingConfig.BriefInfo();
    RobotMgr.BriefInfo();
    HallMgr.BriefInfo();
    GameMgr.BriefInfo();
    DepositHttpMgr.SnapShotObjectStatus();

    // 初始化完毕
    TCHAR hard_id[32];
    xyGetHardID(hard_id);
    LOG_INFO("hard_id [%s]", hard_id);
    g_inited = true;
    return kCommSucc;
}

int AppDelegate::Term() {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    SetEvent(g_hExitServer);

    g_inited = false;

    DepositHttpMgr.Term();
    RobotMgr.Term();
    HallMgr.Term();
    GameMgr.Term();
    SettingConfig.Term();

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
        const auto dwRet = WaitForSingleObject(g_hExitServer, SettingConfig.GetMainsInterval());
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }
        if (WAIT_TIMEOUT == dwRet) {
            if (!g_inited) {
                DEBUG_FUNC;
                continue;
            }
            if (!GameMgr.IsGameDataInited()) {
                DEBUG_FUNC;
                continue;
            };

            // 业务主流程
            MainProcess();
        }
    }

    LOG_INFO("[EXIT] main timer thread [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}

int AppDelegate::MainProcess() {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    DEBUG_FUNC;
    // 获得需要匹配机器人的用户集合
    UserMap need_user_map;
    if (kCommSucc != GetRoomNeedUserMap(need_user_map)) ASSERT_FALSE_RETURN;
    if (need_user_map.empty()) {
        DEBUG_FUNC;
        return kCommSucc;
    }

    // 机器人同步阻塞进入所有指定房间桌子
    DEBUG_FUNC;
    for (auto& kv : need_user_map) {
        const auto userid = kv.first;
        const auto roomid = kv.second->get_room_id();
        const auto tableno = kv.second->get_table_no();

        int64_t max = 0;
        int64_t min = 0;
        GameMgr.GetTableDepositRange(roomid, tableno, max, min);
        int64_t room_max = 0;
        int64_t room_min = 0;
        GameMgr.GetRoomDepositRange(room_max, room_min);
        LOG_INFO("[DEPOSIT] user [%d] roomid [%d] tableno [%d] chairno [%d], room_min [%I64d] room_max [%I64d], table deposit min [%I64d] max [%I64d] need robot",
                 userid, roomid, tableno, kv.second->get_chair_no(), room_min, room_max, min, max);

        // 随机选一个没有进入游戏的机器人userid
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

        // 机器人流程 进入指定房间桌子
        result = RobotProcess(robot_userid, roomid, tableno);
        if (kCommSucc != result) {
            if (kTooLessDeposit == result || kTooMuchDeposit == result) {
                continue;
            }
            if (kNoFreeChair == result) {
                continue;
            }
            if (kAllocTableFaild == result) {
                return result;
            }
            if (kHall_InOtherGame == result) {
                return result;
            }
            if (kHall_UserNotLogon == result) {
                return result;
            }
            ASSERT_FALSE;
        }
    }
    DEBUG_FUNC;
    return kCommSucc;
}

int AppDelegate::RobotProcess(const UserID& userid, const RoomID& roomid, const TableNO& tableno) {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    LOG_ROUTE("beg", roomid, InvalidTableNO, userid);
    CHECK_USERID(userid);
    CHECK_ROOMID(roomid);
    CHECK_ROOMID(tableno);

    // 登陆大厅
    if (kCommSucc != LogonHall(userid)) {
        ASSERT_FALSE_RETURN;
    }

    // LOG
    LOG_ROUTE("enter game", roomid, tableno, userid);
    int64_t deposit = 0;
    DepositDataMgr.GetDeposit(userid, deposit);
    int64_t max = 0;
    int64_t min = 0;
    GameMgr.GetTableDepositRange(roomid, tableno, max, min);
    int64_t room_max = 0;
    int64_t room_min = 0;
    GameMgr.GetRoomDepositRange(room_max, room_min);
    LOG_INFO("[DEPOSIT] enter game robot userid [%d] roomid [%d] tableno [%d] robot current deposit [%I64d] room_min [%I64d] room_max [%I64d], table deposit min [%I64d] max [%I64d] need robot",
             userid, roomid, tableno, deposit, room_min, room_max, min, max);

    // 进入游戏
    const auto result = EnterGame(userid, roomid, tableno);
    if (kCommSucc != result) {
        if (kTooLessDeposit == result || kTooMuchDeposit == result) {
            return result;
        }
        if (kNoFreeChair == result) {
            return result;
        }
        if (kAllocTableFaild == result) {
            return result;
        }
        if (kHall_InOtherGame == result) {
            return result;
        }
        if (kHall_UserNotLogon == result) {
            return result;
        }
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int AppDelegate::LogonHall(const UserID& userid) const {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    CHECK_USERID(userid);
    LOG_ROUTE("LogonHall", InvalidRoomID, InvalidTableNO, userid);

    if (kCommSucc != HallMgr.LogonHall(userid)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int AppDelegate::GetUserGameInfo(const UserID& userid) const {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    CHECK_USERID(userid);
    LOG_ROUTE("GetUserGameInfo", InvalidRoomID, InvalidTableNO, userid);

    if (kCommSucc != HallMgr.GetUserGameInfo(userid)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int AppDelegate::EnterGame(const UserID& userid, const RoomID& roomid, const TableNO& tableno) {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    DEBUG_FUNC;
    CHECK_USERID(userid);
    CHECK_ROOMID(roomid);
    CHECK_TABLENO(tableno);

    LOG_ROUTE("enter game", roomid, tableno, userid);
    const auto game_ip = RobotUtils::GetGameIP();
    const auto game_port = HallMgr.GetGamePort();
    const auto result = RobotMgr.EnterGame(userid, roomid, tableno, game_ip, game_port);

    // 进游戏失败错误处理
    if (kCommSucc !=result) {
        LOG_ERROR("SendEnterGame resp error result [%d] str [%s], roomid [%d] userid [%d]", result, ERR_STR(result), roomid, userid);
        if (kTooLessDeposit == result) {

        } else if (kTooMuchDeposit == result) {

        } else if (kNoFreeChair == result) {

        } else if (kHall_InOtherGame == result) {
            LOG_WARN("userid [%d] kHall_InOtherGame, add to filter list", userid);
            robot_in_other_game_map_[userid] = userid;

        } else if (kHall_UserNotLogon == result) {
            HallMgr.SetLogonStatus(userid, HallLogonStatusType::kNotLogon);

        } else if (result == kInvalidUser) { // 用户不合法（token和entergame时保存的token不一致）
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
    CHECK_MAIN_OR_LAUNCH_THREAD();
    // 过滤 不在游戏的机器人
    // 在游戏的机器人 不做银子操作 容易引起游戏结算错误 业务表现异样
    RobotUserIDMap not_logon_game_temp;
    auto robot_setting = SettingConfig.GetRobotSettingMap();
    for (auto& kv : robot_setting) {
        auto userid = kv.first;
        if (kCommSucc == GameMgr.IsRobotUserExist(userid)) {
            LOG_DEBUG("robot userid [%d] in game", userid);
            continue;
        }
        not_logon_game_temp[userid] = userid;
    }

    // 在游戏中的机器人 


    if (not_logon_game_temp.empty()) {
        LOG_WARN("no more robot in setting pool!");
        return kExceptionNoMoreRobot;
    }

    // 过滤在其他游戏的异常机器人
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

    // 过滤正在补银HTTP请求的机器人
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

    // 过滤正在更新银子信息的机器人
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

    // 过滤房银不符的机器人
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

    // 过滤桌银不符的机器人
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

    // 随机选取userid
    int64_t random_pos = 0;
    if (kCommSucc != RobotUtils::GenRandInRange(0, not_logon_game_temp.size() - 1, random_pos)) {
        ASSERT_FALSE_RETURN;
    }

    const auto random_it = std::next(std::begin(not_logon_game_temp), random_pos);
    random_userid = random_it->first;

    return kCommSucc;
}

int AppDelegate::GetRoomNeedUserMap(UserMap& need_user_map) {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    const int now = std::time(nullptr);
    const auto room_setting_map = SettingConfig.GetRoomSettingMap();

    UserMap filter_user_map;
    if (kCommSucc != GetWaittingUser(filter_user_map)) {
        ASSERT_FALSE_RETURN;
    }

    if (filter_user_map.empty()) {
        return kCommSucc;
    }

    // 根据配置判断是否需要派机器人
    for (auto& kv_setting: room_setting_map) {
        const auto roomid = kv_setting.first;
        const auto setting = kv_setting.second;
        const auto wait_time = setting.wait_time; // seconds
        const auto count_per_table = setting.count_per_table;

        for (const auto& kv_user : filter_user_map) {
            const auto userid = kv_user.first;
            const auto user = kv_user.second;
            const auto tableno = user->get_table_no();

            // 判断桌上机器人是否达到了配置数量 
            auto robot_count_on_chair = 0;
            if (kCommSucc != GameMgr.GetRobotCountOnChairs(roomid, tableno, robot_count_on_chair)) {
                ASSERT_FALSE;
                continue;
            }

            if (robot_count_on_chair >= count_per_table) {
                //LOG_INFO("in count down process reach robot count roomid [%d], tableno [%d], already [%d], design [%d]",
                //roomid, tableno, robot_count_on_chair, count_per_table);
                continue;
            }

            // 判断玩家等待时间，超过配置等待阈值
            ChairInfo info;
            const auto result = GameMgr.GetChairInfo(roomid, tableno, userid, info);
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
    CHECK_MAIN_OR_LAUNCH_THREAD();
    // 获取真实玩家集合 
    UserMap normal_user_map;
    if (kCommSucc != GameMgr.GetNormalUserMap(normal_user_map)) {
        ASSERT_FALSE_RETURN;
    }
    if (normal_user_map.empty()) return kCommSucc;

    for (const auto& kv : normal_user_map) {
        const auto userid = kv.first;
        const auto user = kv.second;
        const auto tableno = user->get_table_no();
        const auto roomid = user->get_room_id();

        if (0 == tableno || 0 == roomid) {
            LOG_DEBUG("userid [%d] tableno [%d] roomid [%d]", userid, tableno, roomid);
            continue;
        }

        // 过滤已开局
        auto is_playing = true;
        if (kCommSucc != GameMgr.IsTablePlaying(roomid, tableno, is_playing)) {
            continue;
        }
        if (is_playing) continue;

        // 过滤非等待玩家
        ChairInfo info;
        const auto result = GameMgr.GetChairInfo(roomid, tableno, userid, info);
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

        // 过滤桌已经达到了最小开桌人数
        auto normal_count_on_chair = 0;
        if (kCommSucc != GameMgr.GetNormalCountOnChairs(roomid, tableno, normal_count_on_chair)) {
            ASSERT_FALSE;
            continue;
        }

        auto min_playercount_per_table = 0;
        if (kCommSucc != GameMgr.GetMiniPlayers(roomid, min_playercount_per_table)) {
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
    CHECK_MAIN_OR_LAUNCH_THREAD();
    LOG_INFO_FUNC("\t[START]");
    const auto robot_map = SettingConfig.GetRobotSettingMap();
    for (auto& kv : robot_map) {
        auto userid = kv.first;
        if (kCommSucc != RobotUtils::IsValidUserID(userid)) {
            ASSERT_FALSE;
            continue;
        }

#ifdef CURRENT_DELAY
        //"正式版防止大量消息冲击后端服务器 [副作用] 正式版机器人启动较慢"
        SLEEP_FOR(CurrentDealy);
#endif

        // 登陆
        if (kCommSucc != LogonHall(userid)) {
            LOG_ERROR("userid [userid] ConnectHallForAllRobot failed", userid);
            ASSERT_FALSE;
            continue;
        }

        // 获得用户信息
        if (kCommSucc != GetUserGameInfo(userid)) {
            LOG_ERROR("userid [userid] ConnectHallForAllRobot failed", userid);
            ASSERT_FALSE;
        }
    }
    return kCommSucc;
}

int AppDelegate::ConnectGameForRobotInGame() {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    LOG_INFO_FUNC("\t[START]");
    const auto users = GameMgr.GetAllUsers();
    for (auto& kv : users) {

#ifdef CURRENT_DELAY
        //"正式版防止大量消息冲击后端服务器 [副作用] 正式版机器人启动较慢"
        SLEEP_FOR(CurrentDealy);
#endif

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
            if (kNoFreeChair == result) {
                continue;
            }
            ASSERT_FALSE;
        }
    }
    return kCommSucc;
}

int AppDelegate::FilterRobotNotInRoomDepositRange(RobotUserIDMap& not_logon_game_temp) const {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    if (not_logon_game_temp.empty()) return kCommFaild;
    int64_t max = 0;
    int64_t min = 0;
    if (kCommSucc != GameMgr.GetRoomDepositRange(max, min)) {
        LOG_WARN("room deposit range min [%I64d] max [%I64d]", min, max);
        return kCommFaild;
    }
    /*LOG_INFO("check robot deposit room deposit range min [%I64d] max [%I64d]", min, max);*/
    auto user_game_info_map = DepositDataMgr.GetUserDepositMap();
    for (const auto& kv: user_game_info_map) {
        const auto userid = kv.first;
        const auto deposit = kv.second;
        if (deposit < 0) ASSERT_FALSE_RETURN;


        // 在游戏中的机器人 不做银子操作 容易引起游戏结算错误 业务表现异样
        if (kCommSucc == GameMgr.IsRobotUserExist(userid)) continue;

        // 必须是机器人
        auto exist = false;
        if (kCommSucc != SettingConfig.IsRobotSettingExist(userid, exist)) {
            continue;
        }
        if (!exist) {
            continue;
        }

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
        }
    }

    return kCommSucc;
}

int AppDelegate::FilterRobotNotInTableDepositRange(const RoomID& roomid, const TableNO& tableno, RobotUserIDMap& not_logon_game_temp) const {
    CHECK_MAIN_OR_LAUNCH_THREAD();
    if (not_logon_game_temp.empty()) return kCommFaild;

    int64_t room_max = 0;
    int64_t room_min = 0;
    if (kCommSucc != GameMgr.GetRoomDepositRange(room_max, room_min)) {
        LOG_WARN("room deposit range room_min [%I64d] room_max [%I64d]", room_min, room_max);
        return kCommFaild;
    }

    int64_t max = 0;
    int64_t min = 0;
    if (kCommSucc != GameMgr.GetTableDepositRange(roomid, tableno, max, min)) {
        ASSERT_FALSE_RETURN;
    }
    if (max < min) ASSERT_FALSE_RETURN;
    if (max <= 0) ASSERT_FALSE_RETURN;
    if (min < 0) ASSERT_FALSE_RETURN;

    if (max > room_max) {
        max = room_max; //桌银上限太高的话 用房银上限代替，防止机器人进不了房间，或者补银失败
    }

    RobotDepositMap too_less_map;
    RobotDepositMap too_much_map;
    auto game_user_info_map = DepositDataMgr.GetUserDepositMap();
    for (const auto& kv : game_user_info_map) {
        const auto userid = kv.first;
        const auto deposit = kv.second;

        // 在游戏中的机器人 不做银子操作 容易引起游戏结算错误 业务表现异样
        if (kCommSucc == GameMgr.IsRobotUserExist(userid)) continue;

        // 必须是机器人
        auto exist = false;
        if (kCommSucc != SettingConfig.IsRobotSettingExist(userid, exist)) {
            continue;
        }
        if (!exist) {
            continue;
        }

        if (deposit < min) { // 桌银区间左闭右开 <
            not_logon_game_temp.erase(userid);
            too_less_map[userid] = deposit;
        }
        if (deposit >= max) { // 桌银区间左闭右开 >=
            not_logon_game_temp.erase(userid);
            too_much_map[userid] = deposit;
        }
    }

    if (not_logon_game_temp.empty()) {
        LOG_WARN("[DEPOSIT] no robot match the roomid [%d] tableno [%d] deposit range min [%I64d] max [%I64d] !", roomid, tableno, min, max);
        // 随机选择1个机器人进行
        // 补银
        if (too_less_map.size() > 0) {
            int64_t random_pos = 0;
            if (kCommSucc != RobotUtils::GenRandInRange(0, too_less_map.size() - 1, random_pos)) {
                ASSERT_FALSE_RETURN;
            }
            const auto random_it = std::next(std::begin(too_less_map), random_pos);
            const auto userid = random_it->first;
            const auto deposit = random_it->second;

            // 在游戏中的机器人 不做银子操作 容易引起游戏结算错误 业务表现异样
            if (kCommSucc == GameMgr.IsRobotUserExist(userid)) return kExceptionRobotNotInGame;

            // 必须是机器人
            auto exist = false;
            if (kCommSucc != SettingConfig.IsRobotSettingExist(userid, exist)) {
                return kExceptionNotRobotType;
            }
            if (!exist) return kExceptionNotRobotType;;

            int64_t random_diff = 0;
            const int64_t range_min = min;
            const int64_t range_max = min + (max -min) * DepoistFactor;
            if (kCommSucc != RobotUtils::GenRandInRange(range_min, range_max, random_diff)) {
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

        // 还银
        if (too_much_map.size() > 0) {
            int64_t amount = 0;
            int64_t random_pos = 0;
            if (kCommSucc != RobotUtils::GenRandInRange(0, too_much_map.size() - 1, random_pos)) {
                ASSERT_FALSE_RETURN;
            }
            const auto random_it = std::next(std::begin(too_much_map), random_pos);
            const auto userid = random_it->first;
            const auto deposit = random_it->second;

            // 在游戏中的机器人 不做银子操作 容易引起游戏结算错误 业务表现异样
            if (kCommSucc == GameMgr.IsRobotUserExist(userid)) return kExceptionRobotNotInGame;

            // 必须是机器人
            auto exist = false;
            if (kCommSucc != SettingConfig.IsRobotSettingExist(userid, exist)) {
                return kExceptionNotRobotType;
            }
            if (!exist) return kExceptionNotRobotType;;

            int64_t random_diff = 0;
            const int64_t range_min = min;
            const int64_t range_max = min + (max -min) * DepoistFactor;
            if (kCommSucc != RobotUtils::GenRandInRange(range_min, range_max, random_diff)) {
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
    // 机器人流程 进入指定房间桌子
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