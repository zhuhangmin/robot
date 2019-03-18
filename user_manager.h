#pragma once
#include "user.h"
#include "robot_define.h"

using UserMap = std::hash_map<UserID, UserPtr>;

using UserFilterMap = std::hash_map<UserID, UserID>;

class UserManager : public ISingletion<UserManager> {
public:

    // 重置对象
    int Reset();

    int ResetDataAndReInit(const game::base::GetGameUsersResp& resp);

    // 获取用户
    // !! 注意 !! 控制线程可见性 只对RobotNetManager notify_thread_ 线程可见
    int GetUser(const UserID& userid, UserPtr& user) const;

    // 删除用户
    int DelUser(const UserID& userid);

    // 添加用户
    int AddUser(const UserID& userid, const UserPtr &user);

    // 获取所有的用户 返回copy 不返回被mutex保护对象引用
    UserMap GetAllUsers() const;

    // 机器人用户是否在游戏
    bool IsRobotUserExist(const UserID& userid) const;

    // 返回调用时所有登入游戏userid集合，不返回引用，返回copy
    UserFilterMap GetAllEnterUserID() const;

    // 获取所有用户数量（正常用户,机器人，各级管理员）
    int GetUserCountInRoom(const RoomID& roomid, int& count) const;

    // 获取所有机器人数量 
    int GetRobotCountInRoom(const RoomID& roomid, int& count) const;

    // 获取所有正常用户数量
    int GetNormalUserCountInRoom(const RoomID& roomid, int& count) const;

    // 获取所有正常用户集合 返回copy 不返回被mutex保护对象引用
    int GetNormalUserMap(UserMap& normal_user_map) const;

    // 获取所有机器人用户集合 返回copy 不返回被mutex保护对象引用
    int GetRobotUserMap(UserMap& robot_user_map) const;

public:
    // 对象状态快照
    int SnapShotObjectStatus();

    int BriefInfo() const;

    int SnapShotUser(const UserID& userid) const;

private:
    int GetUserWithLock(const UserID& userid, UserPtr& user) const;

    int AddUserPBWithLock(const game::base::User& user_pb) const;

    // 添加用户
    int AddUserWithLock(const UserID& userid, const UserPtr &user);

protected:
    SINGLETION_CONSTRUCTOR(UserManager);

private:
    mutable std::mutex mutex_;
    UserMap user_map_;
    ThreadID notify_thread_id_{InvalidThreadID};
};

#define UserMgr UserManager::Instance()
