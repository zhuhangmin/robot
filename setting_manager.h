#pragma once
#include "robot_define.h"
class SettingManager : public ISingletion<SettingManager> {
public:
    int Init();

    int Term();

public:
    // 游戏ID
    GameID GetGameID() const;

    // 机器人账号配置
    const RobotSettingMap& GetRobotSettingMap() const;

    // 机器人房间配置
    const RoomSettingMap& GetRoomSettingMap() const;

    // 获得指定机器人配置
    int GetRobotSetting(UserID userid, RobotSetting& robot_setting) const;

    // 主业务定时器间隔
    int GetMainsInterval() const;

    // 补银定时器间隔
    int GetDepositInterval() const;

    // 补银活动ID
    std::string& GetDepositActiveID();

    // 每次补银数量
    int GetGainAmount() const;

    // 每次还银数量
    int GetBackAmount() const;

    // 补银url
    std::string& GetDepositGainUrl();

    // 还银url
    std::string& GetDepositBackUrl();

public:
    // 对象状态快照
    int SnapShotObjectStatus() const;

    // 获得随机userid
    int GetRandomUserID(UserID& random_userid) const;

    // 启动时是否集体补银
    int GetDeposiInitGainFlag() const;

protected:
    SINGLETION_CONSTRUCTOR(SettingManager);

private:
    int InitSetting();

private:
    RobotSettingMap robot_setting_map_;

    RoomSettingMap room_setting_map_;

    GameID game_id_{InvalidGameID};

    int main_interval_{MainInterval};

    int deposit_interval_{DepositInterval};

    int deposit_init_gain_{InitGainFlag};

    int gain_amount_{GainAmount};

    int back_amount_{BackAmount};

    std::string deposit_active_id_;

    std::string deposit_gain_url_;

    std::string deposit_back_url_;



    //TODO HOT UPDATE NEED RESET AND LOCK?
};

#define SettingMgr  SettingManager::Instance()