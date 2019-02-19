#include "stdafx.h"
#include "common_func.h"


std::string GetConfigFilePath() {
    static std::string config_path = "";

    if (0 == config_path.length()) {
        char root_path[MAX_PATH] = {0};
        GetModuleFileName(NULL, root_path, MAX_PATH);

        config_path = root_path;
        config_path = config_path.substr(0, config_path.find_last_of(('.')));
        config_path += ".ini";
    }

    return config_path;
}

int ParseFromRequest(const REQUEST &req, google::protobuf::Message &val) {
    int repeated = req.head.nRepeated;
    int len = req.nDataLen - repeated * sizeof(CONTEXT_HEAD);
    if (len <= 0) {
        UWL_WRN("Invalid req. repeated = %d, datalen = %d", repeated, req.nDataLen);
        return kCommFaild;
    }

    char *data = (char *) (req.pDataPtr) + repeated * sizeof(CONTEXT_HEAD);
    if (true != val.ParseFromArray(data, len)) {
        UWL_WRN("ParseArray faild. req=%d");
        return kCommFaild;
    }

    return kCommSucc;
}

std::string GetStringFromPb(const google::protobuf::Message& val) {
    std::string msg;
    bool is_true = google::protobuf::TextFormat::PrintToString(val, &msg);
    if (false == is_true) {
        UWL_WRN("PrintToString return false.");
        return msg;
    }

    msg.erase(std::remove(msg.begin(), msg.end(), '\n'), msg.end());
    return msg;
}

//std::string GetStringFromPb_Controlled(const google::protobuf::Message& oMessage) {
//    if (kEnableOff == GameConfig::GetEnable_Pb2String()) {
//        return "";
//    }
//
//    return GetStringFromPb(oMessage);
//}
//
//void MakeUserInfo(const CONTEXT_HEAD &context, const hall::base::EnterGameResp &hall_resp, const game::base::EnterNormalGameReq &enter_req, std::shared_ptr<User> &user) {
//    user->set_user_id(hall_resp.userid());
//    user->set_deposit(hall_resp.deposit());
//    user->set_hard_id(hall_resp.hardid());
//    user->set_head_url(hall_resp.head_url());
//    user->set_nick_name(hall_resp.nick_name());
//    user->set_win_bout(hall_resp.win_bout());
//    user->set_total_bout(hall_resp.total_bout());
//
//    user->set_room_id(enter_req.roomid());
//
//    user->set_sock(context.hSocket);
//    user->set_token(context.lTokenID);
//}
//
//void User2PbUser(const std::shared_ptr<User> &user, game::base::User &pb_user) {
//    pb_user.set_userid(user->get_user_id());
//    pb_user.set_roomid(user->get_room_id());
//    pb_user.set_tableno(user->get_table_no());
//    pb_user.set_chairno(user->get_chair_no());
//    pb_user.set_user_type(user->get_user_type());
//    pb_user.set_deposit(user->get_deposit());
//    pb_user.set_total_bout(user->get_total_bout());
//    pb_user.set_offline_count(user->get_offline_count());
//    pb_user.set_enter_timestamp(user->get_enter_timestamp());
//    pb_user.set_head_url(user->get_head_url());
//    pb_user.set_nick_name(user->get_nick_name());
//}
//
//void Chair2PbChair(int chairno, const ChairInfo &chair, game::base::ChairInfo &pb_chair) {
//    pb_chair.set_chairno(chairno);
//    pb_chair.set_userid(chair.get_userid());
//    pb_chair.set_chair_status(chair.get_chair_status());
//}
//
//void Table2PbTable(const std::shared_ptr<Table> &table, game::base::Table &pb_table) {
//    pb_table.set_roomid(table->get_room_id());
//    pb_table.set_tableno(table->get_table_no());
//    pb_table.set_chair_count(table->get_chair_count());
//    pb_table.set_banker_chair(table->get_banker_chair());
//    pb_table.set_min_deposit(table->get_min_deposit());
//    pb_table.set_max_deposit(table->get_max_deposit());
//    pb_table.set_base_deposit(table->get_base_deposit());
//    pb_table.set_table_status(table->get_table_status());
//
//    std::vector<game::base::ChairInfo> chairs;
//    table->ConstructChair2PbChair(chairs);
//    for (const auto &chair : chairs) {
//        game::base::ChairInfo *pb_chair = pb_table.add_chairs();
//        *pb_chair = chair;
//    }
//
//    std::vector<game::base::TableUserInfo> table_users;
//    table->ConstructTableUser2PbTableUser(table_users);
//    for (const auto &table_user : table_users) {
//        game::base::TableUserInfo *pb_table_user = pb_table.add_table_users();
//        *pb_table_user = table_user;
//    }
//}
//
//void Room2PbRoomData(const std::shared_ptr<BaseRoom> &room, game::base::RoomData &pb_roomdata) {
//    pb_roomdata.set_roomid(room->get_room_id());
//    pb_roomdata.set_chaircount_per_table(room->get_chaircount_per_table());
//    pb_roomdata.set_configs(room->get_configs());
//    pb_roomdata.set_manages(room->get_manages());
//    pb_roomdata.set_options(room->get_options());
//    pb_roomdata.set_max_table_cout(room->get_max_table_cout());
//    pb_roomdata.set_max_deposit(room->get_max_deposit());
//    pb_roomdata.set_min_deposit(room->get_min_deposit());
//}

std::string string_format(const char * _format, ...) {
#define DESCRIB_TEMP 512
    char msg[DESCRIB_TEMP] = {0};

    va_list marker = NULL;
    va_start(marker, _format);

    (void) vsprintf_s(msg, DESCRIB_TEMP - 1, _format, marker);

    va_end(marker);

    return msg;
}
