#pragma once
#include "user.h"
#include "robot_define.h"

using UserMap = std::hash_map<UserID, UserPtr>;

using UserFilterMap = std::hash_map<UserID, UserID>;

class UserManager : public ISingletion<UserManager> {
public:
    // 获取用户的shared_ptr
    int GetUser(const UserID userid, UserPtr& user) const;

    // 获取所有的用户
    const UserMap& GetAllUsers() const;

    // 删除用户
    int DelUser(const UserID userid);

    // 添加用户
    int AddUser(const UserID userid, const UserPtr &user);

    // 重置对象
    int Reset();

public:
    int GetUserWithLock(const UserID userid, UserPtr& user) const;

public:
    // 对象状态快照
    int SnapShotObjectStatus();

    int SnapShotUser(UserID userid) const;

    // 返回调用时所有登入游戏userid集合，不返回引用，返回copy
    UserFilterMap GetAllEnterUserID() const;

    // 获取所有用户数量（正常用户,机器人，各级管理员）
    int GetUserCountInRoom(const RoomID roomid, int& count) const;

    // 获取所有机器人数量 
    int GetRobotCountInRoom(const RoomID roomid, int& count) const;

    // 获取所有正常用户数量
    int GetNormalUserCountInRoom(const RoomID roomid, int& count) const;

protected:
    SINGLETION_CONSTRUCTOR(UserManager);

private:
    mutable std::mutex user_map_mutex_;
    UserMap user_map_;
};

#define UserMgr UserManager::Instance()
