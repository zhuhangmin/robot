#pragma once
#include "user.h"
#include "robot_define.h"

using UserMap = std::hash_map<UserID, UserPtr>;

class UserMgr : public ISingletion<UserMgr> {
public:
    // 获取用户的shared_ptr
    int GetUser(const UserID userid, UserPtr& user) const;
    // 获取所有的用户
    const UserMap& GetAllUsers() const;
    // 删除用户
    int DelUser(const UserID userid);
    // 添加用户
    int AddUser(const UserID userid, const UserPtr &user);

public:
    int Reset();

protected:
    SINGLETION_CONSTRUCTOR(UserMgr);

private:
    mutable std::mutex users_mutex;
    std::hash_map<UserID, std::shared_ptr<User>> users_;
};


