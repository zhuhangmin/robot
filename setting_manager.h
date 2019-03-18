#pragma once
#include "robot_define.h"
class SettingManager : public ISingletion<SettingManager> {
public:
    int Init();

    int Term() const;

public:
    // 游戏ID
    GameID GetGameID() const;

    // 机器人账号配置 支持热更新，不提供受mutex保护的引用，返回copy
    RobotSettingMap GetRobotSettingMap() const;

    // 机器人房间配置 支持热更新，不提供受mutex保护的引用，返回copy
    RoomSettingMap GetRoomSettingMap() const;

    // 获得指定机器人配置 支持热更新，不提供受mutex保护的引用，返回copy
    RobotSetting GetRobotSetting(const UserID& userid) const;

    // 主业务定时器间隔
    int GetMainsInterval() const;

    // 补银定时器间隔
    int GetDepositInterval() const;

    // 补银活动ID
    std::string& GetDepositActiveID();

    // 每次补银数量
    int64_t GetGainAmount() const;

    // 每次还银数量
    int64_t GetBackAmount() const;

    // 补银url
    std::string& GetDepositGainUrl();

    // 还银url
    std::string& GetDepositBackUrl();

    // 热更新
    int ThreadHotUpdate();

    // 检查房间配置是否存在
    int IsRoomSettingExist(const RoomID& roomid, bool& exist);

    // 检查房间配置是否存在
    int IsRobotSettingExist(const UserID& userid, bool& exist);

public:
    // 对象状态快照
    int SnapShotObjectStatus() const;

    int BriefInfo() const;

    // 获得随机userid
    int GetRandomUserID(UserID& random_userid) const;

    // 启动时是否集体补银
    int GetDeposiInitGainFlag() const;

protected:
    SINGLETION_CONSTRUCTOR(SettingManager);

private:
    int InitSettingWithLock();

private:
    mutable std::mutex mutex_;

    YQThread timer_thread_;

    RobotSettingMap robot_setting_map_;

    RoomSettingMap room_setting_map_;

    GameID game_id_{InvalidGameID};

    int main_interval_{MainInterval};

    int deposit_interval_{DepositInterval};

    int deposit_init_gain_{InitGainFlag};

    int64_t gain_amount_{GainAmount};

    int64_t back_amount_{BackAmount};

    std::string deposit_active_id_;

    std::string deposit_gain_url_;

    std::string deposit_back_url_;
};

#define SettingMgr  SettingManager::Instance()