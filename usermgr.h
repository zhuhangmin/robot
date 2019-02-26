#pragma once
#include "user.h"
#include "robot_define.h"

class UserMgr : public ISingletion<UserMgr> {
public:
    // 获取用户的shared_ptr
    int GetUser(int userid, std::shared_ptr<User>& user);
    // 获取所有的用户
    std::hash_map<int, std::shared_ptr<User>> GetAllUsers();
    // 删除用户
    int DelUser(int userid);
    // 添加用户
    int AddUser(int userid, const std::shared_ptr<User> &user);

    bool IsValidUserID(int userid);

public:
    void Reset();

protected:
    SINGLETION_CONSTRUCTOR(UserMgr);

private:
    std::mutex users_mutex;
    std::hash_map<int, std::shared_ptr<User>> users_;
};

