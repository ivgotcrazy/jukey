#include "group-service.h"
#include "log.h"
#include "group-common.h"
#include "protocol.h"
#include "protoc/user.pb.h"
#include "common/util-net.h"
#include "common/util-reporter.h"
#include "common/util-pb.h"
#include "msg-parser.h"
#include "util-protocol.h"


using namespace jukey::com;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
GroupService::GroupService(base::IComFactory* factory, const char* owner)
	: ProxyUnknown(nullptr)
	, ComObjTracer(factory, CID_GROUP_SERVICE, owner)
	, CommonThread("group service", true)
	, m_factory(factory)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* GroupService::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_GROUP_SERVICE) == 0) {
		return new GroupService(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* GroupService::NDQueryInterface(const char* riid)
{
	if (0 == strcmp(riid, IID_SERVICE)) {
		return static_cast<IService*>(this);
	}
	else {
		return ProxyUnknown::NDQueryInterface(riid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool GroupService::DoInitServiceMq()
{
	// Service exchagne
	if (ERR_CODE_OK != m_amqp_client->DeclareExchange(
		m_config.service_config.exchange, ExchangeType::FANOUT)) {
		LOG_ERR("Declare exchange:{} failed!", m_config.service_config.exchange);
		return false;
	}

	std::string service_queue = m_config.service_config.exchange
		+ "_" + m_config.service_config.instance_id;

	// Service queue
	if (ERR_CODE_OK != m_amqp_client->DeclareQueue(service_queue)) {
		LOG_ERR("Declare queue:{} failed!", service_queue);
		return false;
	}

	// Bind service queue to service exchange
	if (ERR_CODE_OK != m_amqp_client->BindQueue(
		m_config.service_config.exchange, service_queue, "")) {
		LOG_ERR("Bind queue:{} to exchange:{} failed!", service_queue,
			m_config.service_config.exchange);
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool GroupService::DoInitNotifyMq()
{
	// Notify exchange
	if (ERR_CODE_OK != m_amqp_client->DeclareExchange(
		m_config.user_config.notify_exchange,
		ExchangeType::FANOUT)) {
		LOG_ERR("Declare notify exchange:{} failed!",
			m_config.user_config.notify_exchange);
		return false;
	}

	// Generate notify queue name
	std::string notify_queue = m_config.user_config.notify_exchange
		+ "_" + m_config.service_config.instance_id;

	// Notify queue
	if (ERR_CODE_OK != m_amqp_client->DeclareQueue(notify_queue)) {
		LOG_ERR("Declare notify queue:{} failed!", notify_queue);
		return false;
	}

	// Bind notify queue to notify exchange
	if (ERR_CODE_OK != m_amqp_client->BindQueue(
		m_config.user_config.notify_exchange, notify_queue, "")) {
		LOG_ERR("Bind ping queue:{} to ping exchange:{} failed!", notify_queue,
			m_config.user_config.notify_exchange);
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool GroupService::DoInitMq()
{
	m_amqp_client = (IAmqpClient*)QI(CID_AMQP_CLIENT, IID_AMQP_CLIENT,
		"group service");
	if (!m_amqp_client) {
		LOG_ERR("Create amqp client failed!");
		return false;
	}

	AmqpParam amqp_param;
	amqp_param.host    = m_config.mq_config.addr.ep.host;
	amqp_param.port    = m_config.mq_config.addr.ep.port;
	amqp_param.user    = m_config.mq_config.user;
	amqp_param.pwd     = m_config.mq_config.pwd;
	amqp_param.handler = this;
	amqp_param.owner   = "group service";

	if (ERR_CODE_OK != m_amqp_client->Init(amqp_param)) {
		LOG_ERR("Initialize amqp client failed!");
		return false;
	}

	if (!DoInitServiceMq()) {
		LOG_ERR("Initialize service mq failed!");
		return false;
	}

	if (!DoInitNotifyMq()) {
		LOG_ERR("Initialize notify ping mq failed!");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool GroupService::DoInitReport()
{
	m_reporter = (IReporter*)QI(CID_REPORTER, IID_REPORTER, "group service");
	if (!m_reporter) {
		LOG_ERR("Create reporter failed!");
		return false;
	}

	com::ReporterParam param;
	param.app = "default";
	param.space = "default";
	param.service_type = "group-service";
	param.instance = m_config.service_config.instance_id;
	param.update_interval = m_config.report_config.interval;
	param.sender.reset(new util::HttpReportSender(
		m_config.report_config.host,
		m_config.report_config.port,
		m_config.report_config.path));

	if (!m_reporter->Init(param)) {
		LOG_ERR("Initialize reporter failed!");
		return false;
	}

	m_reporter->AddReportEntry(DependEntry{
		"rabbitmq",
		"rabbitmq",
		"tcp",
		"mq",
		m_config.mq_config.addr.ToStr(),
		0 });

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool GroupService::Init(net::ISessionMgr* mgr, const std::string& config_file)
{
	auto result = ParseConfig(config_file);
	if (!result.has_value()) {
		LOG_ERR("Parse config failed!");
		return false;
	}
	m_config = result.value();

	if (!DoInitMq()) {
		LOG_ERR("Initialize MQ failed!");
		return false;
	}

	if (!DoInitReport()) {
		LOG_ERR("Initialize report failed!");
		return false;
	}

	m_mq_async_proxy.reset(new util::MqAsyncProxy(
		m_factory, m_amqp_client, this, 10000));

	m_msg_sender.reset(new MsgSender(m_config, m_amqp_client));

	LOG_INF("Initialize success");

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool GroupService::Start()
{
	StartThread();

	m_reporter->Start();

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GroupService::Stop()
{
	m_reporter->Stop();

	StopThread();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool GroupService::Reload()
{
	LOG_INF("Reload");

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GroupService::OnServicePingMsg(const Buffer& mq_buf, const Buffer& sig_buf)
{
	m_msg_sender->SendServicePong(mq_buf, sig_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
UserEntry GroupService::ParseUserEntry(const prot::JoinGroupReq& req)
{
	UserEntry user_entry;
	user_entry.user_type = req.user_type();
	user_entry.user_id = req.user_id();

	for (const auto& entry : req.media_entries()) {
		MediaEntry media_entry;
		media_entry.media_src_type = (MediaSrcType)entry.media_src_type();
		media_entry.media_src_id = entry.media_src_id();
		media_entry.stream_id = entry.stream_id();
		media_entry.stream_type = (StreamType)entry.stream_type();
		media_entry.state = 0;

		user_entry.medias.push_back(media_entry);
	}
	
	return user_entry;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode GroupService::ProcessJoinGroup(const prot::JoinGroupReq& req)
{
	// Create app entry while cannot find it
	auto app_iter = m_app_groups.find(req.app_id());
	if (app_iter == m_app_groups.end()) {
		auto result = m_app_groups.insert(std::make_pair(req.app_id(), AppEntry()));
		if (!result.second) {
			LOG_ERR("Insert app entry failed");
			return ERR_CODE_FAILED;
		}
		LOG_INF("App does not exist, create it");
		app_iter = result.first;
	}

	// Groups in the app
	std::map<uint32_t, GroupEntry>& groups = app_iter->second.groups;

	// Parse user entry from request
	UserEntry user_entry = ParseUserEntry(req);

	auto giter = groups.find(req.group_id());
	if (giter == groups.end()) { // Create a new group entry with incoming user
		GroupEntry group_entry;
		group_entry.users.insert(std::make_pair(req.user_id(), user_entry));
		groups.insert(std::make_pair(req.group_id(), group_entry));
		LOG_INF("First user to join group success");
	}
	else {
		auto uiter = giter->second.users.find(req.user_id());
		if (uiter != giter->second.users.end()) {
			LOG_ERR("User already exists, join group failed");
			return ERR_CODE_FAILED;
		}
		giter->second.users.insert(std::make_pair(req.user_id(), user_entry));
		LOG_INF("Join group success");
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode GroupService::ProcessLeaveGroup(const prot::LeaveGroupReq& req)
{
	// Create app entry while cannot find it
	auto app_iter = m_app_groups.find(req.app_id());
	if (app_iter == m_app_groups.end()) {
		LOG_ERR("Cannot find app");
		return ERR_CODE_FAILED;
	}

	// Groups in the app
	std::map<uint32_t, GroupEntry>& groups = app_iter->second.groups;

	auto giter = groups.find(req.group_id());
	if (giter == groups.end()) { // Create a new group entry with incoming user
		LOG_ERR("Cannot find group");
		return ERR_CODE_FAILED;
	}

	auto uiter = giter->second.users.find(req.user_id());
	if (uiter == giter->second.users.end()) {
		LOG_ERR("Cannot find user");
		return ERR_CODE_FAILED;
	}

	giter->second.users.erase(uiter);
	LOG_INF("Remove user, leave group success");

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::optional<GroupEntry> 
GroupService::GetGroupEntry(uint32_t app_id, uint32_t group_id)
{
	auto app_iter = m_app_groups.find(app_id);
	if (app_iter == m_app_groups.end()) {
		return std::nullopt;
	}

	auto group_iter = app_iter->second.groups.find(group_id);
	if (group_iter == app_iter->second.groups.end()) {
		return std::nullopt;
	}

	return group_iter->second;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GroupService::OnJoinGroupReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	JoinGroupReqPairOpt req_pair = ParseJoinGroupReq(mq_buf, sig_buf);
	if (!req_pair.has_value()) {
		LOG_ERR("Parse join group request failed!");
		return;
	}

	LOG_INF("Received join group request:{}", util::PbMsgToJson(req_pair->second));

	std::map<uint32_t, UserEntry> users;

	if (ERR_CODE_OK != ProcessJoinGroup(req_pair->second)) {
		LOG_ERR("Process join group failed");
		m_msg_sender->SendJoinGroupRsp(mq_buf, sig_buf, req_pair, users,
			ERR_CODE_FAILED, "failed");
		return;
	}

	std::optional<GroupEntry> entry = GetGroupEntry(req_pair->second.app_id(),
		req_pair->second.group_id());
	if (!entry.has_value()) {
		LOG_ERR("Get group entry failed, app:{}, group:{}",
			req_pair->second.app_id(), req_pair->second.group_id());
		m_msg_sender->SendJoinGroupRsp(mq_buf, sig_buf, req_pair, users,
			ERR_CODE_FAILED, "failed");
		return;
	}

	// Filter requester himself
	for (const auto& item : entry->users) {
		if (item.first != req_pair->second.user_id()) {
			users.insert(std::make_pair(item.first, item.second));
		}
	}

	m_msg_sender->SendJoinGroupRsp(mq_buf, sig_buf, req_pair, users, ERR_CODE_OK,
		"success");

	m_msg_sender->SendJoinGroupNotify(mq_buf, sig_buf, req_pair, 
		ParseUserEntry(req_pair->second));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GroupService::OnLeaveGroupReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	LeaveGroupReqPairOpt pair = ParseLeaveGroupReq(mq_buf, sig_buf);
	if (!pair.has_value()) {
		LOG_ERR("Parse leave group request failed!");
		return;
	}

	LOG_INF("Received leave group request:{}", util::PbMsgToJson(pair->second));

	if (ERR_CODE_OK != ProcessLeaveGroup(pair->second)) {
		LOG_ERR("Process leave group failed");
		m_msg_sender->SendLeaveGroupRsp(mq_buf, sig_buf, pair, ERR_CODE_FAILED,
			"failed");
	}
	else {
		m_msg_sender->SendLeaveGroupRsp(mq_buf, sig_buf, pair, ERR_CODE_OK,
			"success");
		m_msg_sender->SendLeaveGroupNotify(mq_buf, sig_buf, pair);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
UserEntry*
GroupService::FindUserEntry(uint32_t app, uint32_t group, uint32_t user)
{
	// app entry
	auto app_iter = m_app_groups.find(app);
	if (app_iter == m_app_groups.end()) {
		LOG_ERR("Cannot find app entry, app:{}", app);
		return nullptr;
	}

	// group entry
	auto grp_iter = app_iter->second.groups.find(group);
	if (grp_iter == app_iter->second.groups.end()) {
		LOG_ERR("Cannot find group entry, group:{}", group);
		return nullptr;
	}

	// user entry
	auto usr_iter = grp_iter->second.users.find(user);
	if (usr_iter == grp_iter->second.users.end()) {
		LOG_ERR("Cannot find user entry, user:{}", user);
		return nullptr;
	}

	return &(usr_iter->second);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GroupService::OnPublishMediaReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	PubMediaReqPairOpt req_pair = ParsePubMediaReq(mq_buf, sig_buf);
	if (!req_pair.has_value()) {
		LOG_ERR("Parse publish media request failed!");
		return; // TODO: response
	}

	LOG_INF("Received publish media request:{}", 
		util::PbMsgToJson(req_pair->second));

	const prot::PublishMediaReq& req = req_pair->second;

	// Find user entry
	UserEntry* ue = FindUserEntry(req.app_id(), req.group_id(), req.user_id());
	if (!ue) {
		LOG_ERR("Failed to find user entry");
		return; // TODO:
	}

	// Find media entry
	auto iter = ue->medias.begin();
	for (; iter != ue->medias.end(); iter++) {
		if ((uint32_t)iter->media_src_type == req.media_entry().media_src_type()
			&& iter->media_src_id == req.media_entry().media_src_id()) {
			break;
		}
	}

	// TODO: no media
	if (iter == ue->medias.end()) {
		MediaEntry entry;
		entry.stream_id = req.media_entry().stream_id();
		entry.stream_type = (StreamType)req.media_entry().stream_type();
		entry.media_src_id = req.media_entry().media_src_id();
		entry.media_src_type = (MediaSrcType)req.media_entry().media_src_type();
		entry.state = 1;

		ue->medias.push_back(entry);

		LOG_INF("Publish none-existant media, add new media:{}|{}",
			req.media_entry().media_src_type(), req.media_entry().media_src_id());
	}
	else {
		if (iter->state == 0) {
			iter->state = 1;
			iter->stream_type = (StreamType)req.media_entry().stream_type();
			iter->stream_id = req.media_entry().stream_id();
			
			LOG_INF("Publish existant media:{}|{}, stream:{}|{}",
				iter->media_src_type,
				iter->media_src_id,
				iter->stream_type,
				iter->stream_id);
		}
		else {
			LOG_WRN("Media has been published"); // TODO:
		}
	}	

	m_msg_sender->SendPublishMediaRsp(mq_buf, sig_buf, req_pair.value());

	m_msg_sender->NotifyPublishMedia(mq_buf, sig_buf, req_pair->second,
		++m_cur_seq);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GroupService::OnUnpublishMediaReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	UnpubMediaReqPairOpt req_pair = ParseUnpubMediaReq(mq_buf, sig_buf);
	if (!req_pair.has_value()) {
		LOG_ERR("Parse unpublish media request failed!");
		return; // TODO: response
	}

	LOG_INF("Received unpublish media request:{}",
		util::PbMsgToJson(req_pair->second));

	const prot::UnpublishMediaReq& req = req_pair->second;

	// Find user entry
	UserEntry* ue = FindUserEntry(req.app_id(), req.group_id(), req.user_id());
	if (!ue) {
		LOG_ERR("Failed to find user entry");
		return; // TODO:
	}

	// Find media entry
	auto iter = ue->medias.begin();
	for (; iter != ue->medias.end(); iter++) {
		if ((uint32_t)iter->media_src_type == req.media_entry().media_src_type()
			&& iter->media_src_id == req.media_entry().media_src_id()) {
			break;
		}
	}

	if (iter == ue->medias.end()) {
		LOG_INF("Publish none-existant media, add new media, type:{}, id:{}",
			req.media_entry().media_src_type(), req.media_entry().media_src_id());
	}

	if (iter->state == 1) {
		iter->state = 0;
	}
	else {
		LOG_WRN("Media has not been published"); // TODO:
	}

	m_msg_sender->SendUnpublishMediaRsp(mq_buf, sig_buf, req_pair.value());
	m_msg_sender->NotifyUnpublishMedia(mq_buf, sig_buf, req_pair->second,
		++m_cur_seq);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GroupService::OnUserOfflineNotify(const Buffer& mq_buf, const Buffer& sig_buf)
{
	prot::UserOfflineNotify notify;
	if (!notify.ParseFromArray(PB_PARSE_SIG_PARAM(sig_buf))) {
		LOG_ERR("Parse user offline notify failed!");
		return;
	}

	LOG_INF("Received user offline notify, msg:{}", util::PbMsgToJson(notify));

	auto app_iter = m_app_groups.find(notify.app_id());
	if (app_iter == m_app_groups.end()) {
		LOG_WRN("Cannot find app:{}", notify.app_id());
		return;
	}

	auto& groups = app_iter->second.groups;

	for (auto grp_iter = groups.begin(); grp_iter != groups.end(); ) {
		auto& users = grp_iter->second.users;

		if (users.find(notify.user_id()) != users.end()) {
			LOG_INF("Remove user:{} from group:{}", notify.user_id(), grp_iter->first);
			users.erase(notify.user_id());

			m_msg_sender->SendLeaveGroupNotify(notify.app_id(), grp_iter->first, 
				notify.user_id(), notify.user_type(), 0);

			if (users.empty()) {
				LOG_INF("Remove group:{} entry", grp_iter->first);
				grp_iter = groups.erase(grp_iter);
			}
			else {
				++grp_iter;
			}
		}
		else {
			++grp_iter;
		}
	}

	if (groups.empty()) {
		LOG_INF("Remove app:{} entry", app_iter->first);
		m_app_groups.erase(app_iter);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GroupService::OnMqMsg(const CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(DoubleBuffer);

	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)(DP(data->buf1));
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)(DP(data->buf2));

	LOG_INF("Received mq message, mq msg:{}, sig msg:{}",
		prot::util::MSG_TYPE_STR(mq_hdr->mt),
		prot::util::MSG_TYPE_STR(sig_hdr->mt));

	prot::util::DumpMqHeader(g_logger, mq_hdr);
	prot::util::DumpSignalHeader(g_logger, sig_hdr);

	switch (sig_hdr->mt) {
	case prot::MSG_SERVICE_PING:
		OnServicePingMsg(data->buf1, data->buf2);
		break;
	case prot::MSG_JOIN_GROUP_REQ:
		OnJoinGroupReq(data->buf1, data->buf2);
		break;
	case prot::MSG_LEAVE_GROUP_REQ:
		OnLeaveGroupReq(data->buf1, data->buf2);
		break;
	case prot::MSG_PUBLISH_MEDIA_REQ:
		OnPublishMediaReq(data->buf1, data->buf2);
		break;
	case prot::MSG_UNPUBLISH_MEDIA_REQ:
		OnUnpublishMediaReq(data->buf1, data->buf2);
		break;
	case prot::MSG_USER_OFFLINE_NOTIFY:
		OnUserOfflineNotify(data->buf1, data->buf2);
		break;
	default:
		if (!m_mq_async_proxy->OnMqMsg(data->buf1, data->buf2)) {
			LOG_ERR("Unknown message type:{}", (uint32_t)sig_hdr->mt);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GroupService::OnThreadMsg(const CommonMsg& msg)
{
	switch (msg.msg_type) {
	case GROUP_MSG_MQ_MSG:
		OnMqMsg(msg);
		break;
	default:
		LOG_ERR("Unknown messge type:{}", msg.msg_type);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GroupService::OnRecvMqMsg(const std::string& queue, const Buffer& mq_buf,
	const Buffer& sig_buf)
{
	LOG_DBG("Receve message from message queue:{}", queue);

	std::shared_ptr<DoubleBuffer> mq_data(new DoubleBuffer(mq_buf, sig_buf));
	PostMsg(CommonMsg(GROUP_MSG_MQ_MSG, mq_data));
}

}