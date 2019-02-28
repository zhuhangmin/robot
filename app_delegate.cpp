#include "stdafx.h"
#include "app_delegate.h"
#include "main.h"
#include "robot_game_manager.h"
#include "setting_manager.h"
#include "game_info_manager.h"
#include "robot_deposit_manager.h"
#include "robot_hall_manager.h"
#include "robot_utils.h"
#include "user_manager.h"
#include "room_manager.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

AppDelegate::AppDelegate() {
    g_hExitServer = NULL;
}

AppDelegate::~AppDelegate() {}

int AppDelegate::InitLanuch() {
    LOG_FUNC("[START ROUTINE]");
    g_hExitServer = CreateEvent(NULL, TRUE, FALSE, NULL);

    TCHAR szFullName[MAX_PATH];
    GetModuleFileName(GetModuleHandle(NULL), szFullName, sizeof(szFullName));
    UwlSplitPath(szFullName, SPLIT_DRIVE_DIR, g_szIniFile);
    g_curExePath = g_szIniFile;
    lstrcat(g_szIniFile, "RobotTool");
    lstrcat(g_szIniFile, _T(".ini"));

    TCHAR szPID[32];
    sprintf_s(szPID, _T("%d"), GetCurrentProcessId());
    WritePrivateProfileString(_T("listen"), _T("pid"), szPID, g_szIniFile);

    g_nClientID = GetPrivateProfileInt(_T("listen"), _T("clientid"), 0, g_szIniFile);
    if (0 == g_nClientID) {
        LOG_ERROR("[START ROUTINE] invalid clientid 0!");
        ASSERT_FALSE;
        return kCommFaild;

    } else {
        LOG_INFO("[START ROUTINE] clientid = [%d]", g_nClientID);
    }
    return kCommSucc;
}

int AppDelegate::Init() {
    LOG_FUNC("[START ROUTINE]");
    if (S_FALSE == ::CoInitialize(NULL))
        return kCommFaild;;

    if (kCommFaild == InitLanuch()) {
        UWL_ERR(_T("InitBase() return failed"));
        ASSERT_FALSE;
        return kCommFaild;
    }

    // 配置数据类
    if (kCommFaild == SettingMgr.Init()) {
        UWL_ERR(_T("SettingManager Init Failed"));
        ASSERT_FALSE;
        return kCommFaild;
    }

    // 机器人大厅管理类
    if (kCommFaild == HallMgr.Init()) {
        UWL_ERR(_T("RobotHallManager Init Failed"));
        ASSERT_FALSE;
        return kCommFaild;
    }

    // 游戏服务数据管理类
    /*auto game_port = RobotUtils::GetGamePort();
    auto game_ip = RobotUtils::GetGameIP();
    if (kCommFaild == GameMgr.Init(game_ip, game_port)) {
    UWL_ERR(_T("RobotGameInfoManager Init Failed"));
    ASSERT_FALSE;
    return kCommFaild;
    }*/

    // 机器人游戏管理类
    if (kCommFaild == RobotMgr.Init()) {
        UWL_ERR(_T("RobotGameManager Init Failed"));
        ASSERT_FALSE;
        return kCommFaild;
    }

    // 机器人补银管理类
    if (kCommFaild == DepositMgr.Init()) {
        UWL_ERR(_T("RobotDepositManager Init Failed"));
        ASSERT_FALSE;
        return kCommFaild;
    }

    // 主流程
    main_timer_thread_.Initial(std::thread([this] {this->ThreadMainProc(); }));

    return kCommSucc;
}

int AppDelegate::Term() {
    SetEvent(g_hExitServer);

    DepositMgr.Term();
    RobotMgr.Term();
    HallMgr.Term();
    GameMgr.Term();
    SettingMgr.Term();

    main_timer_thread_.Release();

    if (g_hExitServer) { CloseHandle(g_hExitServer); g_hExitServer = NULL; }

    ::CoUninitialize();
    LOG_FUNC("[EXIT ROUTINE]");
    LOG_INFO("\n ===================================SERVER EXIT=================================== \n");
    return kCommSucc;

}

int AppDelegate::ThreadMainProc() {
    LOG_INFO("[START ROUTINE] main timer thread [%d] started", GetCurrentThreadId());
    while (true) {
        DWORD dwRet = WaitForSingleObject(g_hExitServer, SettingMgr.GetMainsInterval());
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }
        if (WAIT_TIMEOUT == dwRet) { // timeout

            // 获得此时需要多少机器人进入各个房间
            RoomNeedCountMap room_need_count_map;
            if (kCommSucc != GetRoomNeedCountMap(room_need_count_map)) {
                ASSERT_FALSE;
                continue;
            }

            if (room_need_count_map.size() == 0) {
                continue;
            }

            // 所有房间机器人开始依次同步阻塞进入
            for (auto& kv : room_need_count_map) {
                auto roomid = kv.first;
                auto need_count = kv.second;

                // 随机选一个没有进入游戏的userid
                auto random_userid = InvalidUserID;
                if (kCommSucc != GetRandomUserIDNotInGame(random_userid)) {
                    continue;
                }

                if (random_userid == InvalidUserID) {
                    ASSERT_FALSE;
                    continue;
                }

                if (kCommSucc != RobotProcess(random_userid, roomid)) {
                    continue;
                }

            }
        }
    }

    LOG_INFO("[EXIT ROUTINE] main timer thread [%d] exiting", GetCurrentThreadId());
    return kCommSucc;
}

int AppDelegate::RobotProcess(UserID userid, RoomID roomid) {
    // 登陆大厅
    if (kCommSucc != HallMgr.LogonHall(userid)) {
        ASSERT_FALSE;
        return kCommFaild;
    }

    HallRoomData hall_room_data;
    if (kCommFaild == HallMgr.GetHallRoomData(roomid, hall_room_data)) {
        ASSERT_FALSE;
        return kCommFaild;
    }

    RobotPtr robot;
    if (kCommSucc != RobotMgr.GetRobotWithCreate(userid, robot)) {
        ASSERT_FALSE;
        return kCommFaild;
    }

    if (!robot) {
        ASSERT_FALSE;
        return kCommFaild;
    }

    //@zhuhangmin 20190223 issue: 网络库不支持域名IPV6解析，使用配置IP
    auto game_ip = RobotUtils::GetGameIP();
    auto game_port = RobotUtils::GetGamePort();
    auto game_notify_thread_id = RobotMgr.GetRobotNotifyThreadID();
    if (kCommFaild == robot->ConnectGame(game_ip, game_port, game_notify_thread_id)) {
        ASSERT_FALSE;
        return kCommFaild;
    }

    if (kCommFaild == robot->SendEnterGame(roomid)) {
        ASSERT_FALSE;
        return kCommFaild;
    }

    //TODO 
    // HANDLE EXCEPTION
    // DEPOSIT OVERFLOW UNDERFLOW

    return kCommSucc;
}

int AppDelegate::GetRandomUserIDNotInGame(UserID& random_userid) {
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

    if (not_logon_game_temp.size() == 0) {
        LOG_WARN("no more robot !");
        ASSERT_FALSE;
        return kCommFaild;
    }

    // 随机选取userid
    auto random_pos = 0;
    if (kCommFaild == RobotUtils::GenRandInRange(0, not_logon_game_temp.size() - 1, random_pos)) {
        ASSERT_FALSE;
        return kCommFaild;
    }
    auto random_it = std::next(std::begin(not_logon_game_temp), random_pos);
    random_userid = random_it->first;

    return kCommSucc;
}

int AppDelegate::GetRoomNeedCountMap(RoomNeedCountMap& room_need_count_map) {
    auto room_setting_map = SettingMgr.GetRoomSettingMap();
    for (auto& kv: room_setting_map) {
        auto roomid = kv.first;
        auto setting = kv.second;
        auto designed_count = setting.count;

        RoomPtr room;
        if (kCommSucc != RoomMgr.GetRoom(roomid, room)) {
            ASSERT_FALSE;
            continue;
        }

        int inroom_count = InvalidCount;
        if (kCommSucc != UserMgr.GetRobotCountInRoom(roomid, inroom_count)) {
            ASSERT_FALSE;
            continue;
        }

        if (InvalidCount == inroom_count) {
            ASSERT_FALSE;
            continue;
        }

        auto need_count = designed_count - inroom_count;
        if (need_count > 0) {
            // @zhuhangmin 20190228 每次主循环每个房间只进一个机器人
            room_need_count_map[roomid] = 1;
            // 需要一次上多个机器人使用：
            // room_need_count_map[roomid] = need_count;
        }
    }

    return kCommSucc;
}