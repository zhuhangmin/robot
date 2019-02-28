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
        assert(false);
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
        assert(false);
        return kCommFaild;
    }

    // ����������
    if (kCommFaild == SettingMgr.Init()) {
        UWL_ERR(_T("SettingManager Init Failed"));
        assert(false);
        return kCommFaild;
    }

    // �����˴���������
    if (kCommFaild == HallMgr.Init()) {
        UWL_ERR(_T("RobotHallManager Init Failed"));
        assert(false);
        return kCommFaild;
    }

    // ��Ϸ�������ݹ�����
    /*auto game_port = RobotUtils::GetGamePort();
    auto game_ip = RobotUtils::GetGameIP();
    if (kCommFaild == GameMgr.Init(game_ip, game_port)) {
    UWL_ERR(_T("RobotGameInfoManager Init Failed"));
    assert(false);
    return kCommFaild;
    }*/

    // ��������Ϸ������
    if (kCommFaild == RobotMgr.Init()) {
        UWL_ERR(_T("RobotGameManager Init Failed"));
        assert(false);
        return kCommFaild;
    }

    // �����˲���������
    if (kCommFaild == DepositMgr.Init()) {
        UWL_ERR(_T("RobotDepositManager Init Failed"));
        assert(false);
        return kCommFaild;
    }

    // ������
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

            // ��ô�ʱ��Ҫ���ٻ����˽����������
            RoomNeedCountMap room_need_count_map;
            if (kCommSucc != GetRoomNeedCountMap(room_need_count_map)) {
                assert(false);
                continue;
            }

            if (room_need_count_map.size() == 0) {
                continue;
            }

            // ���з�������˿�ʼ����ͬ����������
            for (auto& kv : room_need_count_map) {
                auto roomid = kv.first;
                auto need_count = kv.second;

                // ���ѡһ��û�н�����Ϸ��userid
                auto random_userid = InvalidUserID;
                if (kCommSucc != GetRandomUserIDNotInGame(random_userid)) {
                    continue;
                }

                if (random_userid == InvalidUserID) {
                    assert(false);
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
    // ��½����
    if (kCommSucc != HallMgr.LogonHall(userid)) {
        assert(false);
        return kCommFaild;
    }

    HallRoomData hall_room_data;
    if (kCommFaild == HallMgr.GetHallRoomData(roomid, hall_room_data)) {
        assert(false);
        return kCommFaild;
    }

    RobotPtr robot;
    if (kCommSucc != RobotMgr.GetRobotWithCreate(userid, robot)) {
        assert(false);
        return kCommFaild;
    }

    if (!robot) {
        assert(false);
        return kCommFaild;
    }

    //@zhuhangmin 20190223 issue: ����ⲻ֧������IPV6������ʹ������IP
    auto game_ip = RobotUtils::GetGameIP();
    auto game_port = RobotUtils::GetGamePort();
    auto game_notify_thread_id = RobotMgr.GetRobotNotifyThreadID();
    if (kCommFaild == robot->ConnectGame(game_ip, game_port, game_notify_thread_id)) {
        assert(false);
        return kCommFaild;
    }

    if (kCommFaild == robot->SendEnterGame(roomid)) {
        assert(false);
        return kCommFaild;
    }

    //TODO 
    // HANDLE EXCEPTION
    // DEPOSIT OVERFLOW UNDERFLOW

    return kCommSucc;
}

int AppDelegate::GetRandomUserIDNotInGame(UserID& random_userid) {
    // ���˳�û�е�����Ϸ��������userid����
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
        assert(false);
        return kCommFaild;
    }

    // ���ѡȡuserid
    auto random_pos = 0;
    if (kCommFaild == RobotUtils::GenRandInRange(0, not_logon_game_temp.size() - 1, random_pos)) {
        assert(false);
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
            ASSERRT_FALSE;
            continue;
        }

        int inroom_count = InvalidCount;
        if (kCommSucc != UserMgr.GetRobotCountInRoom(roomid, inroom_count)) {
            assert(false);
            continue;
        }

        if (InvalidCount == inroom_count) {
            assert(false);
            continue;
        }

        auto need_count = designed_count - inroom_count;
        if (need_count > 0) {
            // @zhuhangmin 20190228 ÿ����ѭ��ÿ������ֻ��һ��������
            room_need_count_map[roomid] = 1;
            // ��Ҫһ���϶��������ʹ�ã�
            // room_need_count_map[roomid] = need_count;
        }
    }

    return kCommSucc;
}