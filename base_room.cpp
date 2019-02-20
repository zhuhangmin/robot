#include "stdafx.h"
#include "base_room.h"

BaseRoom::BaseRoom()
{
	//CreateTables();
}


BaseRoom::BaseRoom(int roomid)
{
	set_room_id(roomid);
	//CreateTables();
}

BaseRoom::~BaseRoom()
{
}

RoomOptional BaseRoom::GetRoomType()
{
	if (IS_BIT_SET(options_, kClassicsRoom)) {
		return kClassicsRoom;
	}
	else if (IS_BIT_SET(options_, kPrivateRoom)) {
		return kPrivateRoom;
	}
	else if (IS_BIT_SET(options_, kMatchRoom)) {
		return kMatchRoom;
	}
	else{
		return kUnkonwRoom;
	}
}

int BaseRoom::PlayerEnterGame(const std::shared_ptr<User> &user)
{
	std::lock_guard<std::mutex> lock_g(alloc_table_chair_mutex_);

	// ��ȡһ����Ч����
	int tableno = GetAvailableTableno(user);

	// �������
	std::shared_ptr<Table> table = GetTable(tableno);
	if (!IsValidTable(table->get_table_no())) {
		LOG_WARN("Get table[%d] faild", tableno);
		return kCommFaild;
	}

	int chairno = 0;
	universal_lock<int> table_lock(tableno, &table_lock_pool_);
	int ret = table->BindPlayer(user, chairno);
	if (ret != kCommSucc) {
		LOG_WARN("BindPlayer faild. userid=%d, tableno=%d", user->get_user_id(), tableno);
		return kCommFaild;
	}

	// ��ʼ����ʱ
	if (table->IsStartGameEnable() && !table->IsCountdowning()) {
		table->StartCountdown();
	}

	// ��ȫMakeUserInfoû�и�ֵ���ֶ�
	user->set_table_no(tableno);
	user->set_chair_no(chairno);
	user->set_enter_timestamp(time(0));

	g_user_mgr.AddUser(user->get_user_id(), user);
	g_user_mgr.AddToken(user->get_token(), user->get_user_id());

	return kCommSucc;
}

int BaseRoom::ContinueGame(const std::shared_ptr<User> &user)
{
	auto table = GetTable(user->get_table_no());
	if (!IsValidTable(table->get_table_no())) {
		LOG_WARN("Get table[%d] faild", user->get_table_no());
		return kCommFaild;
	}

	universal_lock<int> table_lock(user->get_table_no(), &table_lock_pool_);
	// �������ң�У���������Ƿ��Ѿ���������
	if (user->get_chair_no() > 0) {
		int seat_userid = table->GetUserID(user->get_chair_no());
		if (seat_userid != user->get_user_id()) {
			LOG_WARN("user[%d] has in table[%d], chair[%d]. user[%d] continue game faild.", 
				seat_userid, user->get_table_no(), user->get_chair_no(), user->get_user_id());
			return kCommFaild;
		}
	}

	// ���������Ϣ
	g_user_mgr.AddUser(user->get_user_id(), user);
	g_user_mgr.AddToken(user->get_token(), user->get_user_id());
	return kCommSucc;
}

int BaseRoom::UserLeaveGame(int userid, int tableno)
{
	auto table = GetTable(tableno);
	if (!IsValidTable(table->get_table_no())) {
		LOG_WARN("Get table[%d] faild", tableno);
		return kCommFaild;
	}

	{
		universal_lock<int> table_lock(tableno, &table_lock_pool_);
		if (IS_BIT_SET(table->get_table_status(), kTablePlaying)) {
			table->GiveUp(userid);
		}

		table->UnbindUser(userid);

		// ֹͣ����ʱ
		if (!table->IsStartGameEnable() && table->IsCountdowning()) {
			table->StopCountdown();
		}
	}
	return kCommSucc;
}

int BaseRoom::UserGiveUp(int userid, int tableno)
{
	auto table = GetTable(tableno);
	if (!IsValidTable(table->get_table_no())) {
		LOG_WARN("Get table[%d] faild", tableno);
		return kCommFaild;
	}

	{
		universal_lock<int> table_lock(tableno, &table_lock_pool_);
		if (IS_BIT_SET(table->get_table_status(), kTablePlaying)) {
			return table->GiveUp(userid);
		}
	}
	return kCommSucc;
}

// TODO:ȷ��response��code��ͨ������ֵ���ػ���getlasterror����
int BaseRoom::Looker2Player(std::shared_ptr<User> &user)
{
	if (!IsValidDeposit(user->get_deposit())) {
		LOG_WARN("user[%d] deposit[%I64d] invalid.", user->get_user_id(), user->get_deposit());
		return kCommFaild;
	}

	auto table = GetTable(user->get_table_no());
	if (!IsValidTable(table->get_table_no())) {
		LOG_WARN("Get table[%d] faild", user->get_table_no());
		return kCommFaild;
	}

	// ����������
	std::lock_guard<std::mutex> lock_g(alloc_table_chair_mutex_);

	// ������
	universal_lock<int> table_lock(user->get_table_no(), &table_lock_pool_);

	if (!table->IsTableUser(user->get_user_id())) {
		LOG_WARN("user[%d] is not in table[%d]", user->get_user_id(), user->get_table_no());
		return kCommFaild;
	}

	if (table->IsTablePlayer(user->get_user_id())) {
		LOG_WARN("Looker2Player succ. but user[%d] is player.", user->get_user_id());
		// ����������ֱ�ӷ��سɹ�
		user->set_chair_no(table->GetUserChair(user->get_user_id()));
		return kCommSucc;
	}

	int chairno = 0;
	int ret = table->BindPlayer(user, chairno);
	if (ret != kCommSucc) {
		LOG_WARN("user[%d] BindPlayer faild. ret=%d", user->get_user_id(), ret);
		return kCommFaild;
	}

	// ��ʼ����ʱ
	if (table->IsStartGameEnable() && !table->IsCountdowning()) {
		table->StartCountdown();
	}

	user->set_chair_no(chairno);
	return kCommSucc;
}

int BaseRoom::Player2Looker(std::shared_ptr<User> &user)
{
	auto table = GetTable(user->get_table_no());
	if (!IsValidTable(table->get_table_no())) {
		LOG_WARN("Get table[%d] faild", user->get_table_no());
		return kCommFaild;
	}

	// ����������(�������Ҳ���ᵼ�������������ʧ�ܣ������������)
	//std::lock_guard<std::mutex> lock_g(alloc_table_chair_mutex_);

	universal_lock<int> table_lock(user->get_table_no(), &table_lock_pool_);

	if (!table->IsTableUser(user->get_user_id())) {
		LOG_WARN("user[%d] is not in table[%d]", user->get_user_id(), user->get_table_no());
		return kCommFaild;
	}

	table->UnbindPlayer(user->get_user_id());

	// ֹͣ����ʱ
	if (!table->IsStartGameEnable() && table->IsCountdowning()) {
		table->StopCountdown();
	}

	// TODO:�޸�������Ӻ�Ϊ0

	return kCommSucc;
}

int BaseRoom::SwitchTable(std::shared_ptr<User> &user)
{
	// �����ԭ���ӵĹ�ϵ
	{
		auto table = GetTable(user->get_table_no());
		if (!IsValidTable(table->get_table_no())) {
			LOG_WARN("Get table[%d] faild", user->get_table_no());
			return kCommFaild;
		}

		universal_lock<int> table_lock(user->get_table_no(), &table_lock_pool_);
		if (IS_BIT_SET(table->get_table_status(), kTablePlaying) && table->IsTablePlayer(user->get_user_id())) {
			(void)UserGiveUp(user->get_user_id(), user->get_table_no());
		}
		table->UnbindUser(user->get_user_id());

		// ֹͣ����ʱ
		if (!table->IsStartGameEnable() && table->IsCountdowning()) {
			table->StopCountdown();
		}
	}
	
	// ��������
	{
		std::lock_guard<std::mutex> lock_g(alloc_table_chair_mutex_);

		// ��ȡһ����Ч����
		int tableno = GetAvailableTableno(user, user->get_table_no());

		// �������
		auto table = GetTable(tableno);
		if (!IsValidTable(table->get_table_no())) {
			LOG_WARN("Get table[%d] faild", tableno);
			return kCommFaild;
		}

		int chairno = 0;
		universal_lock<int> table_lock(tableno, &table_lock_pool_);
		int ret = table->BindPlayer(user, chairno);
		if (ret != kCommSucc) {
			LOG_WARN("BindPlayer faild. user[%d], table[%d]", user->get_user_id(), tableno);
			return kCommFaild;
		}

		// ��ʼ����ʱ
		if (table->IsStartGameEnable() && !table->IsCountdowning()) {
			table->StartCountdown();
		}

		// �����������Ϣ  TODO:Ҫ����usermanager�����user
		user->set_table_no(tableno);
		user->set_chair_no(chairno);
		user->set_enter_timestamp(time(0));
	}

	return kCommSucc;
}

int BaseRoom::StartGame(int tableno)
{
	std::shared_ptr<Table> table = GetTable(tableno);
	if (false == IsValidTable(table->get_table_no())) {
		LOG_WARN("Get table[%d] faild", tableno);
		return kCommFaild;
	}

	universal_lock<int> table_lock(tableno, &table_lock_pool_);

	// ��table_lock�жԿ�ʼ��Ϸ��������2��У��
	if (!IsStartGameEnable(table)) {
		LOG_WARN("StartGame faild. now=%d, countdown_start=%d, countdown_time_s=%d", time(0), table->get_countdown_timestamp(), get_countdown_time_s());
		return kCommFaild;
	}

	return table->StartGame();
}

int BaseRoom::NotifyChatMsg2TableOthers(int userid, int tableno, const game::base::TableChatNotify &chat_msg)
{
	auto table = GetTable(tableno);
	if (!IsValidTable(table->get_table_no())) {
		LOG_WARN("Get table[%d] faild", tableno);
		return kCommFaild;
	}

	universal_lock<int> table_lock(tableno, &table_lock_pool_);
	table->NotifyUsers(GN_TABLE_CHAT, chat_msg, userid);
	return kCommSucc;
}

int BaseRoom::LookerEnterGame(const std::shared_ptr<User> &user, int target_tableno)
{
	std::shared_ptr<Table> table = GetTable(target_tableno);
	if (false == IsValidTable(table->get_table_no())) {
		LOG_WARN("Get table[%d] faild", target_tableno);
		return kCommFaild;
	}

	universal_lock<int> table_lock(target_tableno, &table_lock_pool_);
	table->BindLooker(user);

	// ��ȫMakeUserInfoû�и�ֵ���ֶ�
	user->set_table_no(target_tableno);
	user->set_enter_timestamp(time(0));

	g_user_mgr.AddUser(user->get_user_id(), user);
	g_user_mgr.AddToken(user->get_token(), user->get_user_id());
	return kCommSucc;
}

int BaseRoom::NotifyStatus2TableOthers(int userid, int tableno, UserStatus user_status)
{
	std::shared_ptr<User> user = g_user_mgr.GetUser(userid);
	//universal_lock<int> user_lock(userid, &g_user_lock_pool);
	if (user->get_user_id() != userid) {
		LOG_WARN("Could not find user[%d]", userid);
		return kCommFaild;
	}

	game::base::Status2ClientNotify notify;
	notify.set_userid(user->get_user_id());
	notify.set_chairno(user->get_chair_no());
	notify.set_user_status(user_status);
	if (user_status == kUserWaiting) {
		notify.set_head_url(user->get_head_url());
		notify.set_nick_name(user->get_nick_name());
	}

	auto table = GetTable(tableno);
	if (!IsValidTable(table->get_table_no())) {
		LOG_WARN("Get table[%d] faild", tableno);
		return kCommFaild;
	}

	universal_lock<int> table_lock(tableno, &table_lock_pool_);
	table->NotifyUsers(GN_USER_STATUS_CHANGE, notify, userid);
	return kCommSucc;
}

int BaseRoom::NotifyStatus2BlackHall(const std::shared_ptr<User> &user, UserStatus user_status)
{
	return kCommSucc;
}

bool BaseRoom::IsValidTable(int tableno)
{
	if (tableno <= 0 || tableno > get_max_table_cout()) {
		return false;
	}
	return true;
}

bool BaseRoom::IsValidDeposit(INT64 deposit)
{
	if (deposit >= get_min_deposit() && deposit < get_max_deposit()) {
		return true;
	}
	return false;
}

void BaseRoom::ConstructEnterGameResponse(const std::shared_ptr<User> &user, game::base::EnterNormalGameResp &resp)
{
	//resp.set_code(kCommSucc);
	resp.set_roomid(user->get_room_id());
	resp.set_tableno(user->get_table_no());
	resp.set_chairno(user->get_chair_no());
	resp.set_deposit(user->get_deposit());

	auto table = GetTable(user->get_table_no());
	if (!IsValidTable(table->get_table_no())) {
		LOG_WARN("Get table[%d] faild", user->get_table_no());
		return ;
	}

	table->ConstructEnterGameResp(user, resp);
}

void BaseRoom::CheckTablesStart()
{
	FUNC_TRACE();

	for (const auto &table : tables_) {

		{
			// TODO:�����Ϊ����ԭ���޷�������ɱ��������ǰ���ȥ���� �˴��������Ӱ���߼�
			universal_lock<int> table_lock(table->get_table_no(), &table_lock_pool_);
			if (!IsStartGameEnable(table)) {
				continue;
			}
		}

		game::base::StartGameReq req;
		req.set_roomid(get_room_id());
		req.set_tableno(table->get_table_no());

		CONTEXT_HEAD context = {};
		(void)g_sock_server.PutRequest2Worker(GR_START_GAME, context, req);
	}
}

void BaseRoom::ContrustTable2PbTable(std::vector<game::base::Table> &tables)
{
	for (int i = 0; i < get_max_table_cout() && i < kMaxTableCountPerRoom; ++i) {
		game::base::Table pb_table;
		Table2PbTable(tables_.at(i), pb_table);
		tables.push_back(pb_table);
	}
}

bool BaseRoom::IsStartGameEnable(const std::shared_ptr<Table> &table)
{
	int time_cost = time(0) - table->get_countdown_timestamp();
	if (table->get_countdown_timestamp() == 0 || time_cost < get_countdown_time_s() || !table->IsStartGameEnable()) {
		LOG_TRACE("StartGame faild. start_time_s=%d, countdown_time_s=%d", time_cost, get_countdown_time_s());
		return false;
	}

	return true;
}

std::shared_ptr<Table> BaseRoom::GetTable(int tableno)
{
	static std::shared_ptr<Table> null_table = std::make_shared<Table>();

	if (tableno <= 0 || tableno > max_table_cout_) {
		LOG_WARN("GetTable faild. valid tableno[%d]", tableno);
		return null_table;
	}

	// ���Ŵ�1��ʼ���±��0��ʼ
	return tables_.at(tableno - 1);
}

int BaseRoom::GetAvailableTableno(const std::shared_ptr<User> &user, int shielding_tableno/* = 0*/)
{
	const int faild_tableno = 0;
	int min_tableno = 0, max_tableno = 0;
	int ret = GetEligibleTable(user, min_tableno, max_tableno);
	if (ret != kCommSucc) {
		LOG_WARN("GetEligibleTable faild.");
		return faild_tableno;
	}

	// �±�Ϊ���ӵĿ���λ����valueΪ���Ӻ�
	std::array<int, kMaxChairCountPerTable + 1> available_tables = {};

	// ɨ�跶Χ���������ӣ��ҳ���ͬ���ȼ��Ŀ������Ӻ�
	for (int tableno = min_tableno; tableno < max_tableno; ++tableno){
		if (tableno == shielding_tableno) {
			continue;
		}

		auto table = GetTable(tableno);
		if (!IsValidTable(table->get_table_no())) {
			continue;
		}

		universal_lock<int> table_lock(tableno, &table_lock_pool_);

		if (!IsValidDeposit(user->get_deposit())) {
			continue;
		}

		int free_chair_count = table->GetFreeChairCount();
		if (free_chair_count <= 0 || free_chair_count > kMaxChairCountPerTable || free_chair_count > chaircount_per_table_) {
			LOG_WARN("GetFreeChairCount return [%d]", free_chair_count);
			continue;
		}

		available_tables.at(free_chair_count) = tableno;

		// �ҵ�ֻ��һ������λ�����ӾͲ������ˣ�ֱ������
		if (1 == free_chair_count) {
			break;
		}
	}

	// ѡ�����շ������ҵ�����
	for (int i = 0; i < available_tables.size(); ++i){
		if (available_tables.at(i) <= 0) {
			continue;
		}

		return available_tables.at(i);
	}

	return faild_tableno;
}

int BaseRoom::GetEligibleTable(const std::shared_ptr<User> &user, int &min_tableno, int &max_tableno)
{
	// ��Ϸ���ɸ����Լ��������ش˺���������˵�������Ƶȣ�

	min_tableno = 1;	// ���Ŵ�1��ʼ
	max_tableno = max_table_cout_;

	if (min_tableno <= 0 || max_tableno <= 0 || max_tableno - min_tableno <= 0) {
		return kCommFaild;
	}

	return kCommSucc;
}

std::shared_ptr<Table> BaseRoom::NewTable(int tableno)
{
	return std::make_shared<Table>(tableno, get_room_id(), get_chaircount_per_table(), 
		get_min_playercount_per_table(), get_base_deposit());
}

void BaseRoom::CreateTables()
{
	for (int i = 0; i < sizeof(tables_); ++i) {
		tables_[i] = NewTable(i + 1);
	}
}

int BaseRoom::SetUserTableInfo(int userid, int tableno, int chairno)
{
	auto user = g_user_mgr.GetUser(userid);
	if (user->get_user_id() <= 0) {
		LOG_WARN("SetUserTableInfo get user[%d] faild.", userid);
		return kCommFaild;
	}

	universal_lock<int> userlock(userid, &g_user_lock_pool);
	user->set_table_no(tableno);
	user->set_chair_no(chairno);
	user->set_enter_timestamp(time(0));
	return kCommSucc;
}
