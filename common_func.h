#pragma once

// ��ȡ�����ļ�(ini)��·��
std::string GetConfigFilePath();

// ��REQUEST����pb�ṹ
int ParseFromRequest(const REQUEST &req, google::protobuf::Message &val);

// pb�ṹתΪ�ַ���
std::string GetStringFromPb(const google::protobuf::Message& oMessage);
//// pb�ṹתΪ�ַ���(�ܿ������ÿ���)
//std::string GetStringFromPb_Controlled(const google::protobuf::Message& oMessage);

//// ����User��������Դ��EnterGameResp��EnterGameReq
//void MakeUserInfo(const CONTEXT_HEAD &context, const hall::base::EnterGameResp &hall_resp, const game::base::EnterNormalGameReq &enter_req, std::shared_ptr<User> &user);
//
//// User�����ݿ�����pbЭ�鶨���User��
//void User2PbUser(const std::shared_ptr<User> &user, game::base::User &pb_user);
//
//// Chair�����ݿ�����pbЭ�鶨���Chair��
//void Chair2PbChair(int chairno, const ChairInfo &chair, game::base::ChairInfo &pb_chair);
//
//// Table�����ݿ�����pbЭ�鶨���Table��
//void Table2PbTable(const std::shared_ptr<Table> &table, game::base::Table &pb_table);
//
//// Room�����ݿ�����pbЭ�鶨���RoomData��
//void Room2PbRoomData(const std::shared_ptr<BaseRoom> &room, game::base::RoomData &pb_roomdata);

// �ַ�����ʽ���� ע�⣺�ڲ����泤��Ϊ1024������ַ������ȳ���1024��������ʹ�ô˺���������
std::string string_format(const char * _format, ...);