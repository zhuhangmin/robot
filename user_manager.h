#pragma once
#include "user.h"
#include "robot_define.h"

using UserMap = std::hash_map<UserID, UserPtr>;

using UserFilterMap = std::hash_map<UserID, UserID>;

class UserManager : public ISingletion<UserManager> {
public:

    // ���ö���
    int Reset();

    int ResetDataAndReInit(const game::base::GetGameUsersResp& resp);

    // ��ȡ�û�
    // !! ע�� !! �����߳̿ɼ��� ֻ��RobotNetManager notify_thread_ �߳̿ɼ�
    int GetUser(const UserID& userid, UserPtr& user) const;

    // ɾ���û�
    int DelUser(const UserID& userid);

    // ����û�
    int AddUser(const UserID& userid, const UserPtr &user);

    // ��ȡ���е��û� ����copy �����ر�mutex������������
    UserMap GetAllUsers() const;

    // �������û��Ƿ�����Ϸ
    bool IsRobotUserExist(const UserID& userid) const;

    // ���ص���ʱ���е�����Ϸuserid���ϣ����������ã�����copy
    UserFilterMap GetAllEnterUserID() const;

    // ��ȡ�����û������������û�,�����ˣ���������Ա��
    int GetUserCountInRoom(const RoomID& roomid, int& count) const;

    // ��ȡ���л��������� 
    int GetRobotCountInRoom(const RoomID& roomid, int& count) const;

    // ��ȡ���������û�����
    int GetNormalUserCountInRoom(const RoomID& roomid, int& count) const;

    // ��ȡ���������û����� ����copy �����ر�mutex������������
    int GetNormalUserMap(UserMap& normal_user_map) const;

    // ��ȡ���л������û����� ����copy �����ر�mutex������������
    int GetRobotUserMap(UserMap& robot_user_map) const;

public:
    // ����״̬����
    int SnapShotObjectStatus();

    int BriefInfo() const;

    int SnapShotUser(const UserID& userid) const;

private:
    int GetUserWithLock(const UserID& userid, UserPtr& user) const;

    int AddUserPBWithLock(const game::base::User& user_pb) const;

    // ����û�
    int AddUserWithLock(const UserID& userid, const UserPtr &user);

protected:
    SINGLETION_CONSTRUCTOR(UserManager);

private:
    mutable std::mutex mutex_;
    UserMap user_map_;
    ThreadID notify_thread_id_{InvalidThreadID};
};

#define UserMgr UserManager::Instance()
