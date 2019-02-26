#pragma once
#include "user.h"
#include "robot_define.h"

using UserMap = std::hash_map<UserID, UserPtr>;

class UserMgr : public ISingletion<UserMgr> {
public:
    // ��ȡ�û���shared_ptr
    int GetUser(const UserID userid, UserPtr& user) const;
    // ��ȡ���е��û�
    const UserMap& GetAllUsers() const;
    // ɾ���û�
    int DelUser(const UserID userid);
    // ����û�
    int AddUser(const UserID userid, const UserPtr &user);

public:
    int Reset();

protected:
    SINGLETION_CONSTRUCTOR(UserMgr);

private:
    mutable std::mutex users_mutex;
    std::hash_map<UserID, std::shared_ptr<User>> users_;
};


