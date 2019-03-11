#include "stdafx.h"
#include "app_delegate.h"
#include "main.h"
#include "robot_net_manager.h"
#include "setting_manager.h"
#include "game_net_manager.h"
#include "robot_deposit_manager.h"
#include "robot_hall_manager.h"
#include "robot_utils.h"
#include "user_manager.h"
#include "room_manager.h"

int AppDelegate::InitLanuch() const {
    LOG_INFO_FUNC("[START]");
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
        LOG_ERROR("[START] invalid clientid 0!");
        ASSERT_FALSE_RETURN;

    }
    LOG_INFO("[START] clientid = [%d]", g_nClientID);
    return kCommSucc;
}

int AppDelegate::Init() {
    LOG_INFO_FUNC("[START]");
    if (S_FALSE == CoInitialize(NULL))
        ASSERT_FALSE_RETURN;

    // ！！【注意】 不可调换顺序或者并行初始化 ！！
    // 以下数据类初始化有依赖关系，故使用同步io，
    if (kCommSucc != InitLanuch()) {
        LOG_ERROR(_T("InitBase() return failed"));
        ASSERT_FALSE_RETURN;
    }

    // 配置数据类
    if (kCommSucc != SettingMgr.Init()) {
        LOG_ERROR(_T("SettingManager Init Failed"));
        ASSERT_FALSE_RETURN;
    }

    // 机器人大厅管理类
    if (kCommSucc != HallMgr.Init()) {
        LOG_ERROR(_T("RobotHallManager Init Failed"));
        ASSERT_FALSE_RETURN;
    }

    // 游戏服务数据管理类
    auto game_port = RobotUtils::GetGamePort();
    auto game_ip = RobotUtils::GetGameIP();
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

    // 机器人补银管理类
    if (kCommSucc != DepositMgr.Init()) {
        LOG_ERROR(_T("RobotDepositManager Init Failed"));
        ASSERT_FALSE_RETURN;
    }

    // 以上数据管理类均为线程安全

    // 是否启动时集体补银
    DepositGainAll();

    // ！！【注意】请勿使用多线程构建业务！！
    // 请[只在 ThreadMainProc 单线程] 实现调度
    // 其他线程构建业务会出现线程竞争问题
    main_timer_thread_.Initial(std::thread([this] {this->ThreadMainProc(); }));

    // 初始化完毕
    inited_ = true;
    return kCommSucc;
}

int AppDelegate::Term() {
    SetEvent(g_hExitServer);

    inited_ = false;

    DepositMgr.Term();
    RobotMgr.Term();
    HallMgr.Term();
    GameMgr.Term();
    SettingMgr.Term();

    main_timer_thread_.Release();

    if (g_hExitServer) { CloseHandle(g_hExitServer); g_hExitServer = NULL; }

    CoUninitialize();
    LOG_INFO_FUNC("[EXIT ROUTINE]");
    LOG_INFO("\n ===================================SERVER EXIT=================================== \n");
    return kCommSucc;

}

int AppDelegate::ThreadMainProc() {
    LOG_INFO("[START] main timer thread [%d] started", GetCurrentThreadId());

    while (true) {
        const auto dwRet = WaitForSingleObject(g_hExitServer, SettingMgr.GetMainsInterval());
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }
        if (WAIT_TIMEOUT == dwRet) {
            MainProcess();
        }
    }

    LOG_INFO("[EXIT ROUTINE] main timer thread [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}

int AppDelegate::MainProcess() {
    if (!inited_) return kCommSucc;
    // 获得需要匹配机器人的用户集合
    UserMap need_user_map;
    if (kCommSucc != GetRoomNeedUserMap(need_user_map)) {
        ASSERT_FALSE_RETURN;
    }

    if (need_user_map.empty()) {
        LOG_INFO("NO NEED TO ROUTE ROBOT");
        return kCommSucc;
    }

    // 机器人同步阻塞进入所有指定房间桌子
    for (auto& kv : need_user_map) {
        const auto userid = kv.first;
        const auto user = kv.second;
        const auto roomid = user->get_room_id();
        const auto tableno = user->get_table_no();
        const auto chairno = user->get_chair_no();
        LOG_INFO("user [%d] roomid [%d] tableno [%d] chairno [%d] need robot",
                 userid, roomid, tableno, chairno);

        // 随机选一个没有进入游戏的userid
        auto robot_userid = InvalidUserID;
        if (kCommSucc != GetRandomUserIDNotInGame(robot_userid)) {
            ASSERT_FALSE;
            continue;
        }
        if (InvalidUserID == robot_userid) {
            ASSERT_FALSE;
            continue;
        }

        // 机器人流程 进入指定房间桌子
        if (kCommSucc != RobotProcess(robot_userid, roomid, tableno)) {
            ASSERT_FALSE;
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

    if (kCommSucc != EnterGame(userid, roomid, tableno)) {
        ASSERT_FALSE_RETURN;
    }

    return kCommSucc;
}

int AppDelegate::LogonHall(const UserID& userid) {
    CHECK_USERID(userid);
    LOG_ROUTE("LogonHall", InvalidRoomID, InvalidTableNO, userid);

    if (kCommSucc != HallMgr.LogonHall(userid)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int AppDelegate::EnterGame(const UserID& userid, const RoomID& roomid, const TableNO& tableno) {
    CHECK_USERID(userid);
    CHECK_ROOMID(roomid);
    CHECK_TABLENO(tableno);

    LOG_ROUTE("get robot", roomid, tableno, userid);
    RobotPtr robot;
    if (kCommSucc != RobotMgr.GetRobotWithCreate(userid, robot)) {
        ASSERT_FALSE_RETURN;
    }
    if (!robot) {
        ASSERT_FALSE_RETURN;
    }

    LOG_ROUTE("connect game", roomid, tableno, userid);
    //@zhuhangmin 20190223 issue: 网络库不支持域名IPV6解析，使用配置IP
    const auto game_ip = RobotUtils::GetGameIP();
    const auto game_port = RobotUtils::GetGamePort();
    const auto game_notify_thread_id = RobotMgr.GetNotifyThreadID();
    if (kCommSucc != robot->Connect(game_ip, game_port, game_notify_thread_id)) {
        ASSERT_FALSE_RETURN;
    }

    LOG_ROUTE("enter game", roomid, tableno, userid);
    const auto result = robot->SendEnterGame(roomid, tableno);
    if (kCommSucc !=result) {
        LOG_ERROR("SendEnterGame resp error result [%d] str [%s], roomid [%d] userid [%d]", result, ERR_STR(result), roomid, userid);
        if (kTooLessDeposit == result) {
            DepositMgr.SetDepositType(userid, DepositType::kGain);

        } else if (kTooMuchDeposit == result) {
            DepositMgr.SetDepositType(userid, DepositType::kBack);

        } else {
            ASSERT_FALSE_RETURN;

        }

        if (kHall_UserNotLogon == result) {
            HallMgr.SetLogonStatus(userid, HallLogonStatusType::kNotLogon);
        }

        LOG_ROUTE("!!! ENTER GAME FAILED !!!", roomid, tableno, userid);
        ASSERT_FALSE_RETURN;
    }
    LOG_ROUTE("*** ENTER GAME SUCCESS***", roomid, tableno, userid);
    return kCommSucc;
}

int AppDelegate::GetRandomUserIDNotInGame(UserID& random_userid) const {
    // 过滤出没有登入游戏服务器的userid集合
    std::hash_map<UserID, UserID> not_logon_game_temp;
    auto enter_game_map = UserMgr.GetAllEnterUserID();
    auto robot_setting = SettingMgr.GetRobotSettingMap();
    for (auto& kv : robot_setting) {
        auto userid = kv.first;
        if (enter_game_map.find(userid) == enter_game_map.end()) {
            not_logon_game_temp[userid] = userid;
        }
    }

    if (not_logon_game_temp.empty()) {
        LOG_WARN("no more robot !");
        ASSERT_FALSE_RETURN;
    }

    // 随机选取userid
    auto random_pos = 0;
    if (kCommSucc != RobotUtils::GenRandInRange(0, not_logon_game_temp.size() - 1, random_pos)) {
        ASSERT_FALSE_RETURN;
    }
    const auto random_it = std::next(std::begin(not_logon_game_temp), random_pos);
    random_userid = random_it->first;

    return kCommSucc;
}

//TODO 当主循环MainProces时间小于机器人配桌等待时间应判断是否有机器人已经入桌
//TODO 判断桌子状态 和 桌子上的人 和 机器人数量
//TODO 延迟误差 如果一个玩家在一个主循环刚刚开始时进入，需要等待到下个主循环
//TODO 缩小主循环可以减少误差，但要注意多个机器人上同一个桌
int AppDelegate::GetRoomNeedUserMap(UserMap& need_user_map) {
    const int now = time(nullptr);
    const auto room_setting_map = SettingMgr.GetRoomSettingMap();

    //获取真实玩家集合
    //TODO OFFICIAL
    /*UserMap normal_user_map;
    if (kCommSucc != UserMgr.GetNormalUserMap(normal_user_map)) {
    ASSERT_FALSE_RETURN;
    }*/

    // TODO TEST ONLY
    UserMap normal_user_map;
    if (kCommSucc != UserMgr.GetRobotUserMap(normal_user_map)) {
        ASSERT_FALSE_RETURN;
    }
    // TODO TEST ONLY

    if (normal_user_map.empty()) return kCommSucc;

    for (auto& kv: room_setting_map) {
        const auto roomid = kv.first;
        const auto setting = kv.second;
        const auto wait_time = setting.wait_time; // seconds
        const auto count_per_table = setting.count_per_table;

        RoomPtr room;
        if (kCommSucc != RoomMgr.GetRoom(roomid, room)) {
            ASSERT_FALSE;
            continue;
        }

        //判断每个玩家的等待时间，超过设计等待阈值的添加匹配机器人
        for (const auto& kv : normal_user_map) {
            const auto userid = kv.first;
            const auto user = kv.second;
            const auto tableno = user->get_table_no();

            //已经在桌上的机器人是否达到了设计数量
            auto robot_count_on_chair = 0;
            if (kCommSucc != RoomMgr.GetRobotCountOnChairs(roomid, tableno, robot_count_on_chair)) {
                ASSERT_FALSE;
                continue;
            }

            if (robot_count_on_chair >= count_per_table) {
                LOG_INFO("reach robot count roomid [%d], tableno [%d], already [%d], design [%d]",
                         roomid, tableno, robot_count_on_chair, count_per_table);
                continue;
            }


            ChairInfo info;
            if (kCommSucc != RoomMgr.GetChairInfo(roomid, tableno, userid, info)) {
                ASSERT_FALSE;
                continue;
            }
            int bind = info.get_bind_timestamp();
            if (0 == bind) {
                //TODO OFFICIAL
                /*    LOG_WARN("userid [%d] tableno [%d] roomid [%d] bind 0", userid, tableno, roomid);
                    ASSERT_FALSE;
                    continue;*/

                // TEST ONLY
                bind = 1552307211;
                // TEST ONLY
            }
            int elapse = now - bind;
            if (elapse >= wait_time) {
                auto now_date = RobotUtils::TimeStampToDate(now);
                auto bind_date = RobotUtils::TimeStampToDate(bind);
                LOG_INFO("userid [%d] now [%d][%s] bind [%d][%s] elapse [%d]",
                         userid, now, now_date.c_str(), bind, bind_date.c_str(), elapse);

                need_user_map[userid] = user;
            }
        }
    }

    return kCommSucc;
}

int AppDelegate::DepositGainAll() {
    if (InitGainFlag  != SettingMgr.GetDeposiInitGainFlag())
        return kCommSucc;

    auto robot_setting_map = SettingMgr.GetRobotSettingMap();
    for (auto& kv : robot_setting_map) {
        const auto userid = kv.first;
        DepositMgr.SetDepositType(userid, DepositType::kGain);
    }

    return kCommSucc;
}

int AppDelegate::ConnectHallForAllRobot() {
    LOG_INFO_FUNC("[START]");
    const auto robot_map = SettingMgr.GetRobotSettingMap();
    for (auto& kv : robot_map) {
        auto userid = kv.first;
        if (kCommSucc != RobotUtils::IsValidUserID(userid)) {
            ASSERT_FALSE;
            continue;
        }

#ifndef _DEBUG
        LOG_WARN("正式版防止大量消息冲击后端服务器 [副作用] 正式版机器人启动较慢");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
#endif

        if (kCommSucc != LogonHall(userid)) {
            LOG_ERROR("userid [userid] ConnectHallForAllRobot failed", userid);
            ASSERT_FALSE;
            continue;
        }
    }
    return kCommSucc;
}

int AppDelegate::ConnectGameForRobotInGame() {
    LOG_INFO_FUNC("[START]");
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

        auto user_tpye = user->get_user_type();
        if (kUserRobot != user_tpye) {
            ASSERT_FALSE;
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

        if (kCommSucc != EnterGame(userid, roomid, tableno)) {
            LOG_ERROR("userid [userid] tableno [%d] roomid [%d]  ConnectGameForRobotInGame failed", userid, tableno, roomid);
            ASSERT_FALSE;
            continue;
        }
    }
    return kCommSucc;
}

// TEST ONLY
void AppDelegate::SendTestRobot(const UserID& userid, const RoomID& roomid, const TableNO& tableno) {
    // 机器人流程 进入指定房间桌子
    if (kCommSucc != RobotProcess(userid, roomid, tableno)) {
        ASSERT_FALSE;
    }
}

int AppDelegate::TestLogonHall(const UserID& userid) {
    CHECK_USERID(userid);
    LOG_ROUTE("LogonHall", InvalidRoomID, InvalidTableNO, userid);

    if (kCommSucc != HallMgr.LogonHall(userid)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}