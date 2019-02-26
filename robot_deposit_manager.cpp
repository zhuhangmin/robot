#include "stdafx.h"
#include "robot_deposit_manager.h"
#include "setting_manager.h"
#include "Main.h"
#include "robot_utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#define ROBOT_APPLY_DEPOSIT_KEY "zjPUYq9L36oA9zke"

int RobotDepositManager::Init() {

    deposit_timer_thread_.Initial(std::thread([this] {this->ThreadDeposit(); }));

    UWL_INF("RobotDepositManager::Init Sucessed.");
    return kCommSucc;
}


void RobotDepositManager::Term() {
    deposit_timer_thread_.Release();
}

void RobotDepositManager::ThreadDeposit() {
    UWL_INF(_T("Hall Deposit thread started. id = %d"), SettingMgr.GetDepositInterval());

    while (true) {
        DWORD dwRet = WaitForSingleObject(g_hExitServer, DepositInterval);
        if (WAIT_OBJECT_0 == dwRet) {
            break;
        }

        if (WAIT_TIMEOUT == dwRet) {
            DepositMap deposit_map_temp;
            {
                std::lock_guard<std::mutex> lock(deposit_map_mutex_);
                deposit_map_temp = std::move(deposit_map_);
            }

            for (auto& kv : deposit_map_temp) {
                auto userid = kv.first;
                auto deposit_type = kv.second;

                if (deposit_type == DepositType::kGain) {
                    RobotGainDeposit(userid, SettingMgr.GetGainAmount());
                }

                if (deposit_type == DepositType::kBack) {
                    RobotBackDeposit(userid, SettingMgr.GetBackAmount());
                }
            }
        }
    }

    UWL_INF(_T("Hall Deposit thread exiting. id = %d"), GetCurrentThreadId());
    return;
}


int RobotDepositManager::RobotGainDeposit(UserID userid, int amount) {
    CHECK_USERID(userid);
    if (amount <= 0) {
        assert(false);
        return kCommFaild;
    }
    static TCHAR szHttpUrl[128];
    static int nResult = GetPrivateProfileString(_T("robot_depoist"), _T("deposit_gain_url"), _T("null"), szHttpUrl, sizeof(szHttpUrl), g_szIniFile);
    if (0 == strcmp(szHttpUrl, "null"))
        return kCommFaild;

    GameID game_id = SettingMgr.GetGameID();
    CHECK_GAMEID(game_id);
    TCHAR szValue[128] = {};
    GetPrivateProfileString(_T("robot_depoist"), "deposit_active_id", _T("null"), szValue, sizeof(szValue), g_szIniFile);
    if (0 == strcmp(szValue, "null"))
        return kCommFaild;

    Json::Value root;
    Json::FastWriter fast_writer;
    root["GameId"] = Json::Value(game_id);
    root["ActId"] = Json::Value(szValue);
    root["UserId"] = Json::Value(userid);
    root["UserNickName"] = Json::Value("Robot");
    root["Num"] = Json::Value(amount);
    root["Operation"] = Json::Value(1);
    root["Device"] = Json::Value(xyConvertIntToStr(game_id) + "+" + xyConvertIntToStr(userid));
    root["Time"] = Json::Value((LONGLONG) time(NULL) * 1000);
    root["Sign"] = Json::Value(MD5String((root["ActId"].asString() + "|" + root["UserId"].asString() + "|" + root["Time"].asString() + "|" + ROBOT_APPLY_DEPOSIT_KEY).c_str()));

    CString strParam = fast_writer.write(root).c_str();
    CString strResult = RobotUtils::ExecHttpRequestPost(szHttpUrl, strParam);
    Json::Reader reader;
    Json::Value _root;
    if (!reader.parse((LPCTSTR) strResult, _root))
        return kCommFaild;
    if (_root["Code"].asInt() != 0) {
        UWL_ERR("userid = %d gain deposit fail, code = %d, strResult = %s", userid, _root["Code"].asInt(), strResult);
        //assert(false);
        return kCommFaild;
    }

    return kCommSucc;
}

int RobotDepositManager::RobotBackDeposit(UserID userid, int amount) {
    CHECK_USERID(userid);
    if (amount <= 0) {
        assert(false);
        return kCommFaild;
    }
    static TCHAR szHttpUrl[128];
    static int nResult = GetPrivateProfileString(_T("robot_depoist"), _T("deposit_back_url"), _T("null"), szHttpUrl, sizeof(szHttpUrl), g_szIniFile);
    if (0 == strcmp(szHttpUrl, "null"))
        return kCommFaild;

    GameID game_id = SettingMgr.GetGameID();
    CHECK_GAMEID(game_id);
    TCHAR szValue[128] = {};
    GetPrivateProfileString(_T("robot_depoist"), "deposit_active_id", _T("null"), szValue, sizeof(szValue), g_szIniFile);
    if (0 == strcmp(szValue, "null"))
        return kCommFaild;

    Json::Value root;
    Json::FastWriter fast_writer;
    root["GameId"] = Json::Value(game_id);
    root["ActId"] = Json::Value(szValue);
    root["UserId"] = Json::Value(userid);
    root["UserNickName"] = Json::Value("Robot");
    root["Num"] = Json::Value(amount);
    root["Operation"] = Json::Value(2);
    root["Device"] = Json::Value(xyConvertIntToStr(game_id) + "+" + xyConvertIntToStr(userid));
    root["Time"] = Json::Value((LONGLONG) time(NULL) * 1000);
    root["Sign"] = Json::Value(MD5String((root["ActId"].asString() + "|" + root["UserId"].asString() + "|" + root["Time"].asString() + "|" + ROBOT_APPLY_DEPOSIT_KEY).c_str()));

    CString strParam = fast_writer.write(root).c_str();
    CString strResult = RobotUtils::ExecHttpRequestPost(szHttpUrl, strParam);
    Json::Reader reader;
    Json::Value _root;
    if (!reader.parse((LPCTSTR) strResult, _root))
        return kCommFaild;
    if (!_root["Status"].asBool())
        return kCommFaild;
    return kCommSucc;
}

int RobotDepositManager::GetDepositTypeWithLock(const UserID& userid, DepositType& type) {
    CHECK_USERID(userid);
    if (deposit_map_.find(userid) == deposit_map_.end()) {
        return kCommFaild;
    }

    type = deposit_map_[userid];
    return kCommSucc;
}

int RobotDepositManager::SetDepositTypesWithLock(const UserID userid, DepositType type) {
    CHECK_USERID(userid);
    deposit_map_[userid] = type;
    return kCommSucc;
}