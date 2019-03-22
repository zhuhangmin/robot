#pragma once
#include "robot_define.h"

// 不支持热更新是只读immutable对象,可以保证多线程安全
// 一旦支持热更新 需要考虑加锁，对象可见层次，上层缓存失效等问题
class SettingManager : public ISingletion<SettingManager> {
public:
    int Init();

    int Term() const;

public:
    // 游戏ID
    GameID GetGameID() const;

    // 机器人账号配置 
    RobotSettingMap GetRobotSettingMap() const;

    // 机器人房间配置 
    RoomSettingMap GetRoomSettingMap() const;

    // 获得指定机器人配置 
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

    // 定时消息
    int ThreadTimer();

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

protected:
    SINGLETION_CONSTRUCTOR(SettingManager);

private:
    int InitSettingWithLock();

private:
    // 机器人配置
    RobotSettingMap robot_setting_map_;

    // 机器人房间配置
    RoomSettingMap room_setting_map_;

    // 游戏ID
    GameID game_id_{InvalidGameID};

    // 业务主线程定时器间隔
    int main_interval_{MainInterval};

    // 补银单线程定时器间隔
    int deposit_interval_{DepositInterval};

    // 默认单次补银数量
    int64_t gain_amount_{GainAmount};

    // 默认单次还银数量
    int64_t back_amount_{BackAmount};

    // 后台配置补银还银活动ID
    std::string deposit_active_id_;

    // 后台补银服务地址
    std::string deposit_gain_url_;

    // 后台还银服务地址
    std::string deposit_back_url_;
};

#define SettingMgr  SettingManager::Instance()