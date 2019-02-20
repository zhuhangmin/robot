#pragma once
#include "user.h"

class UserMgr {
public:
    UserMgr();
    virtual ~UserMgr();

    // 新建一个用户
    std::shared_ptr<User> NewUser();
    // 获取用户的shared_ptr
    std::shared_ptr<User> GetUser(int userid);
    // 获取用户副本的shared_ptr
    std::shared_ptr<User> GetUserCopy(int userid);
    // 获取所有的用户
    std::hash_map<int, std::shared_ptr<User>> GetAllUsers();
    // 删除用户
    void DelUser(int userid);
    // 添加用户
    void AddUser(int userid, const std::shared_ptr<User> &user);


    std::shared_ptr<User> GetUserByToken(int token);
    int GetUseridByToken(int token);
    void DeleteToken(int token);
    void AddToken(int token, int userid);

    bool IsValidUserID(int userid);

private:
    std::mutex users_mutex;
    std::hash_map<int, std::shared_ptr<User>> users_;

    std::mutex tokens_mutex;
    std::hash_map<int, int> tokens_;
};

