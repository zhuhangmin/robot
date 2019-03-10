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
    LOG_INFO_FUNC("[SVR START]");
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
        LOG_ERROR("[SVR START] invalid clientid 0!");
        ASSERT_FALSE_RETURN;

    }
    LOG_INFO("[SVR START] clientid = [%d]", g_nClientID);
    return kCommSucc;
}

int AppDelegate::Init() {
    LOG_INFO_FUNC("[SVR START]");
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
    LOG_INFO("[SVR START] main timer thread [%d] started", GetCurrentThreadId());

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
    // 获得此时需要多少机器人进入各个房间
    RoomNeedCountMap room_need_count_map;
    if (kCommSucc != GetRoomNeedCountMap(room_need_count_map)) {
        ASSERT_FALSE_RETURN;
    }

    if (room_need_count_map.empty()) {
        //LOG_INFO("NO NEED TO ROUTE ROBOT");
        return kCommSucc;
    } else {
        for (auto& kv : room_need_count_map) {
            LOG_INFO("room [%d] need robot [%d]", kv.first, kv.second);
        }
    }

    // 所有房间机器人开始依次同步阻塞进入
    for (auto& kv : room_need_count_map) {
        const auto roomid = kv.first;
        const auto need_count = kv.second;
        for (auto index = 0; index < need_count; index++) {
            // 随机选一个没有进入游戏的userid
            auto random_userid = InvalidUserID;
            if (kCommSucc != GetRandomUserIDNotInGame(random_userid)) {
                continue;
            }
            if (random_userid == InvalidUserID) {
                ASSERT_FALSE_RETURN;
            }

            // 机器人流程
            if (kCommSucc != RobotProcess(random_userid, roomid)) {
                ASSERT_FALSE_RETURN;
            }
        }
    }
    return kCommSucc;
}

int AppDelegate::RobotProcess(const UserID& userid, const RoomID& roomid) {
    LOG_ROUTE("beg", roomid, InvalidTableNO, userid);
    CHECK_USERID(userid);
    CHECK_ROOMID(roomid);

    if (kCommSucc != LogonHall(userid)) {
        ASSERT_FALSE_RETURN;
    }

    if (kCommSucc != EnterGame(userid, roomid)) {
        ASSERT_FALSE_RETURN;
    }

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

int AppDelegate::GetRoomNeedCountMap(RoomNeedCountMap& room_need_count_map) {
    auto room_setting_map = SettingMgr.GetRoomSettingMap();
    for (auto& kv: room_setting_map) {
        const auto roomid = kv.first;
        const auto setting = kv.second;
        const auto designed_count = setting.count;

        //TODO COMMENT BACK LATER
        /*RoomPtr room;
        if (kCommSucc != RoomMgr.GetRoom(roomid, room)) {
        ASSERT_FALSE_RETURN;
        }*/

        auto inroom_count = 0;
        if (kCommSucc != UserMgr.GetRobotCountInRoom(roomid, inroom_count)) {
            ASSERT_FALSE_RETURN;
        }

        const auto need_count = designed_count - inroom_count;
        if (need_count > 0) {
            // 每次循环每个房间只进一个机器人
            room_need_count_map[roomid] = 1;
            // 需要一次上多个机器人使用：
            //room_need_count_map[roomid] = need_count;
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
    const auto robot_map = SettingMgr.GetRobotSettingMap();
    for (auto& kv : robot_map) {
        auto userid = kv.first;
        if (kCommSucc != RobotUtils::IsValidUserID(userid)) continue;

#ifndef _DEBUG
        LOG_WARN("正式版防止大量消息冲击后端服务器 [副作用] 正式版机器人启动较慢");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
#endif

        if (kCommSucc != LogonHall(userid)) {
            LOG_ERROR("userid [userid] ConnectHallForAllRobot failed", userid);
            continue;
        }
    }
    return kCommSucc;
}

int AppDelegate::ConnectGameForRobotInGame() {
    LOG_INFO("ConnectGameForRobotInGame");
    const auto users = UserMgr.GetAllUsers();
    for (auto& kv : users) {
        auto userid = kv.first;
        auto user = kv.second;
        if (kCommSucc != RobotUtils::IsValidUserID(userid)) continue;
        if (kCommSucc != RobotUtils::IsValidUser(user)) continue;

        auto roomid = user->get_room_id();
        auto tableno = user->get_table_no();
        if (kCommSucc != RobotUtils::IsValidRoomID(roomid)) continue;
        if (kCommSucc != RobotUtils::IsValidTableNO(tableno)) continue;

        if (kCommSucc != EnterGame(userid, roomid, tableno)) {
            LOG_ERROR("userid [userid] tableno [%d] roomid [%d]  ConnectGameForRobotInGame failed", userid, tableno, roomid);
            continue;
        }
    }
    return kCommSucc;
}


int AppDelegate::LogonHall(UserID userid) {
    CHECK_USERID(userid);
    LOG_ROUTE("ConnectHallForAllRobot", InvalidRoomID, InvalidTableNO, userid);

    if (kCommSucc != HallMgr.LogonHall(userid)) {
        ASSERT_FALSE_RETURN;
    }
    return kCommSucc;
}

int AppDelegate::EnterGame(UserID userid, RoomID roomid, TableNO tableno) {
    CHECK_USERID(userid);
    CHECK_ROOMID(roomid);

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
    //TODO 需要大厅提供错误码
    const auto result = robot->SendEnterGame(roomid);
    if (kCommSucc !=result) {
        LOG_ERROR("SendEnterGame resp error result [%d] str [%s], roomid [%d] userid [%d]", result, ERR_STR(result), roomid, userid);

        // 银子不够or太多，设置标签，补银还银线程处理 TODO
        /*if (kDepositUnderFlow == resp_code) {
        DepositMgr.SetDepositType(userid, DepositType::kGain);

        } else if (kDepositOverFlow == resp_code) {
        DepositMgr.SetDepositType(userid, DepositType::kBack);

        } else {
        ASSERT_FALSE_RETURN;

        }*/

        if (kHall_UserNotLogon == result) {
            HallMgr.SetLogonStatus(userid, HallLogonStatusType::kNotLogon);
        }

        LOG_ROUTE("!!! ENTER GAME SUCCESS !!!", roomid, tableno, userid);
        //TODO
        //ASSERT_FALSE_RETURN;
        // return kCommFaild;
        return kCommSucc;
    }
    LOG_ROUTE("*** ENTER GAME SUCCESS***", roomid, tableno, userid);
    return kCommSucc;
}