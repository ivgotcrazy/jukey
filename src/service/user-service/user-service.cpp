#include "user-service.h"
#include "log.h"
#include "protocol.h"
#include "user-common.h"
#include "common/util-net.h"
#include "common/util-reporter.h"
#include "common/util-time.h"
#include "common/util-pb.h"
#include "msg-parser.h"
#include "util-protocol.h"

using namespace jukey::com;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
UserService::UserService(base::IComFactory* factory, const char* owner)
	: ProxyUnknown(nullptr)
	, ComObjTracer(factory, CID_USER_SERVICE, owner)
	, CommonThread("user service", true)
	, m_factory(factory)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* UserService::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_USER_SERVICE) == 0) {
		return new UserService(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* UserService::NDQueryInterface(const char* riid)
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
bool UserService::DoInitReport()
{
	m_reporter = (IReporter*)QI(CID_REPORTER, IID_REPORTER, "user service");
	if (!m_reporter) {
		LOG_ERR("Create reporter failed!");
		return false;
	}

	com::ReporterParam param;
	param.app = "default";
	param.space = "default";
	param.service_type = "user-service";
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
	"mq", "rabbitmq", "tcp", "mq", m_config.mq_config.addr.ToStr(), 0 });

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool UserService::DoInitServiceMq()
{
	// Service exchagne
	if (ERR_CODE_OK != m_amqp_client->DeclareExchange(
		m_config.service_config.exchange, ExchangeType::FANOUT)) {
		LOG_ERR("Declare service exchange failed!");
		return false;
	}

	std::string service_queue = m_config.service_config.exchange + "_" +
		m_config.service_config.instance_id;

	// Service queue
	if (ERR_CODE_OK != m_amqp_client->DeclareQueue(service_queue)) {
		LOG_ERR("Declare service queue failed!");
		return false;
	}

	// Bind service queue to service exchange
	if (ERR_CODE_OK != m_amqp_client->BindQueue(m_config.service_config.exchange,
		service_queue, "")) {
		LOG_ERR("Bind service queue failed!");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool UserService::DoInitNotifyMq()
{
	// User service notify exchange
	if (ERR_CODE_OK != m_amqp_client->DeclareExchange(
		m_config.service_config.notify_exchange,
		ExchangeType::FANOUT)) {
		LOG_ERR("Declare notify exchange failed!");
		return false;
	}

	// Terminal service notify exchange
	if (ERR_CODE_OK != m_amqp_client->DeclareExchange(
		m_config.terminal_config.notify_exchange,
		ExchangeType::FANOUT)) {
		LOG_ERR("Declare terminal notify exchange failed!");
		return false;
	}

	std::string notify_queue = m_config.terminal_config.notify_exchange + "_" +
		m_config.service_config.instance_id;

	// Service queue
	if (ERR_CODE_OK != m_amqp_client->DeclareQueue(notify_queue)) {
		LOG_ERR("Declare notify queue failed!");
		return false;
	}

	// Bind service queue to service exchange
	if (ERR_CODE_OK != m_amqp_client->BindQueue(
		m_config.terminal_config.notify_exchange, notify_queue, "")) {
		LOG_ERR("Bind queue failed!");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool UserService::DoInitMq()
{
	m_amqp_client = (IAmqpClient*)QI(CID_AMQP_CLIENT, IID_AMQP_CLIENT,
		"user service");
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
	amqp_param.owner   = "user service";

	if (ERR_CODE_OK != m_amqp_client->Init(amqp_param)) {
		LOG_ERR("Initialize amqp client failed!");
		return false;
	}

	if (!DoInitServiceMq()) {
		LOG_ERR("Initialize service mq failed");
		return false;
	}

	if (!DoInitNotifyMq()) {
		LOG_ERR("Initialize notify mq failed");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool UserService::Init(net::ISessionMgr* mgr, const std::string& config_file)
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

	m_msg_sender.reset(new MsgSender(m_config, m_amqp_client));

	LOG_INF("Initialize success");

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool UserService::Start()
{
	StartThread();

	m_reporter->Start();

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void UserService::Stop()
{
	m_reporter->Stop();

	StopThread();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool UserService::Reload()
{
	LOG_INF("Reload");

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void UserService::OnUserLoginReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	UserLoginReqPairOpt req_pair = ParseUserLoginReq(mq_buf, sig_buf);
	if (!req_pair.has_value()) {
		LOG_ERR("Parse user login request failed!");
		return; // TODO:
	}

	LOG_INF("Received login request:{}", util::PbMsgToJson(req_pair->second));

	if (m_users.find(req_pair->second.user_id()) != m_users.end()) {
		LOG_ERR("User already exists");
		m_msg_sender->SendUserLoginRsp(mq_buf, sig_buf, req_pair, 0,
			ERR_CODE_FAILED, "user already exists");
		return;
	}

	UserEntrySP entry(new UserEntry());
	entry->app_id = req_pair->second.app_id();
	entry->user_id = req_pair->second.user_id();
	entry->client_id = req_pair->second.client_id();
	entry->user_type = req_pair->second.user_type();
	entry->register_id = req_pair->second.register_id();
	entry->login_id = ++m_login_id;
	entry->login_time = util::Now();

	m_users.insert(std::make_pair(entry->user_id, entry));

	LOG_INF("Add user:{}", entry->user_id);

	m_msg_sender->SendUserLoginRsp(mq_buf, sig_buf, req_pair, m_login_id,
		ERR_CODE_OK, "success");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void UserService::OnUserLogoutReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	UserLogoutReqPairOpt req_pair = ParseUserLogoutReq(mq_buf, sig_buf);
	if (!req_pair.has_value()) {
		LOG_ERR("Parse user login request failed!");
		return; // TODO:
	}

	LOG_INF("Received logout request:{}", util::PbMsgToJson(req_pair->second));

	auto iter = m_users.find(req_pair->second.user_id());
	if (iter == m_users.end()) {
		LOG_ERR("Cannot find user");
		m_msg_sender->SendUserLogoutRsp(mq_buf, sig_buf, req_pair, ERR_CODE_FAILED,
			"cannot find user");
		return;
	}

	if (iter->second->login_id != req_pair->second.login_id()) {
		LOG_ERR("Login ID mismatch");
		m_msg_sender->SendUserLogoutRsp(mq_buf, sig_buf, req_pair, ERR_CODE_FAILED,
			"login ID mismatch");
		return;
	}

	m_msg_sender->SendUserLogoutRsp(mq_buf, sig_buf, req_pair, ERR_CODE_OK,
		"success");

	m_msg_sender->SendUserOfflineNotify(++m_cur_seq, iter->second);

	m_users.erase(iter);
	LOG_INF("Remove user");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void UserService::OnClientOfflineNotify(const Buffer& mq_buf, const Buffer& sig_buf)
{
	ClientOfflineNotifyPairOpt pair = ParseClientOfflineNotify(mq_buf, sig_buf);
	if (!pair.has_value()) {
		LOG_ERR("Parse client offline notify failed!");
		return; // TODO:
	}

	LOG_INF("Received client offline notify:{}", util::PbMsgToJson(pair->second));

	for (auto iter = m_users.begin(); iter != m_users.end(); iter++) {
		if (iter->second->client_id == pair->second.client_id()
			&& iter->second->register_id == pair->second.register_id()) {
			m_msg_sender->SendUserOfflineNotify(++m_cur_seq, iter->second);
			LOG_INF("Remove user:{}", iter->first);
			m_users.erase(iter);
			return;
		}
	}

	LOG_ERR("Cannot find matched user to remove");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void UserService::OnServicePingMsg(const Buffer& mq_buf, const Buffer& sig_buf)
{
	ServicePingPairOpt ping_pair = ParseServicePing(mq_buf, sig_buf);
	if (!ping_pair.has_value()) {
		LOG_ERR("Parse service ping failed");
		return;
	}

	LOG_INF("Received service ping:{}", util::PbMsgToJson(ping_pair->second));

	m_msg_sender->SendServicePong(mq_buf, sig_buf, ping_pair);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void UserService::OnMqMsg(const CommonMsg& msg)
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
	case prot::MSG_USER_LOGIN_REQ:
		OnUserLoginReq(data->buf1, data->buf2);
		break;
	case prot::MSG_USER_LOGOUT_REQ:
		OnUserLogoutReq(data->buf1, data->buf2);
		break;
	case prot::MSG_CLIENT_OFFLINE_NOTIFY:
		OnClientOfflineNotify(data->buf1, data->buf2);
		break;
	default:
		LOG_ERR("Unknown message type:{}", msg.msg_type);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void UserService::OnThreadMsg(const CommonMsg& msg)
{
	switch (msg.msg_type) {
	case USER_MSG_MQ_MSG:
		OnMqMsg(msg);
		break;
	default:
		LOG_ERR("Unknown messge type:{}", msg.msg_type);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void UserService::OnRecvMqMsg(const std::string& queue, const Buffer& mq_buf,
	const Buffer& sig_buf)
{
	std::shared_ptr<DoubleBuffer> mq_data(new DoubleBuffer(mq_buf, sig_buf));
	PostMsg(CommonMsg(USER_MSG_MQ_MSG, mq_data));
}

}