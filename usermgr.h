#pragma once
#include "user.h"
#include "robot_define.h"

class UserMgr : public ISingletion<UserMgr> {
public:
    // 获取用户的shared_ptr
    std::shared_ptr<User> GetUser(int userid);
    // 获取所有的用户
    std::hash_map<int, std::shared_ptr<User>> GetAllUsers();
    // 删除用户
    void DelUser(int userid);
    // 添加用户
    void AddUser(int userid, const std::shared_ptr<User> &user);

    bool IsValidUserID(int userid);

public:
    void Reset();

protected:
    SINGLETION_CONSTRUCTOR(UserMgr);

private:
    std::mutex users_mutex;
    std::hash_map<int, std::shared_ptr<User>> users_;
};

