#pragma once
#include "user.h"

class UserMgr {
public:
    UserMgr();
    virtual ~UserMgr();

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

