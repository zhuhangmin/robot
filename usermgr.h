#pragma once
#include "user.h"
#include "RobotDef.h"

class UserMgr : public ISingletion<UserMgr> {
public:
    // �½�һ���û�
    std::shared_ptr<User> NewUser();
    // ��ȡ�û���shared_ptr
    std::shared_ptr<User> GetUser(int userid);
    // ��ȡ�û�������shared_ptr
    std::shared_ptr<User> GetUserCopy(int userid);
    // ��ȡ���е��û�
    std::hash_map<int, std::shared_ptr<User>> GetAllUsers();
    // ɾ���û�
    void DelUser(int userid);
    // ����û�
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

