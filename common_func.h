#pragma once

// 获取配置文件(ini)的路径
std::string GetConfigFilePath();

// 从REQUEST解析pb结构
int ParseFromRequest(const REQUEST &req, google::protobuf::Message &val);

// pb结构转为字符串
std::string GetStringFromPb(const google::protobuf::Message& oMessage);
//// pb结构转为字符串(受开关配置控制)
//std::string GetStringFromPb_Controlled(const google::protobuf::Message& oMessage);

//// 生成User，数据来源于EnterGameResp和EnterGameReq
//void MakeUserInfo(const CONTEXT_HEAD &context, const hall::base::EnterGameResp &hall_resp, const game::base::EnterNormalGameReq &enter_req, std::shared_ptr<User> &user);
//
//// User的数据拷贝到pb协议定义的User中
//void User2PbUser(const std::shared_ptr<User> &user, game::base::User &pb_user);
//
//// Chair的数据拷贝到pb协议定义的Chair中
//void Chair2PbChair(int chairno, const ChairInfo &chair, game::base::ChairInfo &pb_chair);
//
//// Table的数据拷贝到pb协议定义的Table中
//void Table2PbTable(const std::shared_ptr<Table> &table, game::base::Table &pb_table);
//
//// Room的数据拷贝到pb协议定义的RoomData中
//void Room2PbRoomData(const std::shared_ptr<BaseRoom> &room, game::base::RoomData &pb_roomdata);

// 字符串格式化。 注意：内部缓存长度为1024，如果字符串长度超过1024，则不允许使用此函数！！！
std::string string_format(const char * _format, ...);