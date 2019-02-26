#pragma once
#include "user.h"
#include "robot_define.h"

class UserMgr : public ISingletion<UserMgr> {
public:
    // ��ȡ�û���shared_ptr
    int GetUser(int userid, std::shared_ptr<User>& user);
    // ��ȡ���е��û�
    std::hash_map<int, std::shared_ptr<User>> GetAllUsers();
    // ɾ���û�
    int DelUser(int userid);
    // ����û�
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

