#pragma once
#include "user.h"
#include "robot_define.h"

using UserMap = std::hash_map<UserID, UserPtr>;

using UserFilterMap = std::hash_map<UserID, UserID>;

class UserManager : public ISingletion<UserManager> {
public:
    // ��ȡ�û���shared_ptr
    int GetUser(const UserID& userid, UserPtr& user) const;

    // ��ȡ���е��û� ����copy �����ر�mutex������������
    // �������� ��ʹ����ʵʱ��Ҫ��ߵ�ҵ��
    UserMap GetAllUsers() const;

    // ɾ���û�
    int DelUser(const UserID& userid);

    // ����û�
    int AddUser(const UserID& userid, const UserPtr &user);

    // ���ö���
    int Reset();

public:
    int GetUserWithLock(const UserID& userid, UserPtr& user) const;

public:
    // ����״̬����
    int SnapShotObjectStatus();

    int UserManager::BriefInfo() const;

    int SnapShotUser(const UserID& userid) const;

    // ���ص���ʱ���е�����Ϸuserid���ϣ����������ã�����copy
    UserFilterMap GetAllEnterUserID() const;

    // ��ȡ�����û������������û�,�����ˣ���������Ա��
    int GetUserCountInRoom(const RoomID& roomid, int& count) const;

    // ��ȡ���л��������� 
    int GetRobotCountInRoom(const RoomID& roomid, int& count) const;

    // ��ȡ���������û�����
    int GetNormalUserCountInRoom(const RoomID& roomid, int& count) const;

    // ��ȡ���������û�����
    int GetNormalUserMap(UserMap& normal_user_map) const;

    // ��ȡ���л������û�����
    int GetRobotUserMap(UserMap& robot_user_map) const;

protected:
    SINGLETION_CONSTRUCTOR(UserManager);

private:
    mutable std::mutex mutex_;
    UserMap user_map_;
};

#define UserMgr UserManager::Instance()
