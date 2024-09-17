#include "terminal-service.h"
#include "log.h"
#include "common-define.h"
#include "terminal-common.h"
#include "common/util-net.h"
#include "common/util-time.h"
#include "common/util-reporter.h"
#include "common/util-pb.h"
#include "util-protocol.h"
#include "msg-parser.h"


using namespace jukey::com;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
TerminalService::TerminalService(base::IComFactory* factory, const char* owner)
	: ProxyUnknown(nullptr)
	, ComObjTracer(factory, CID_TERMINAL_SERVICE, owner)
	, CommonThread("terminal service", true)
	, m_factory(factory)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* TerminalService::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_TERMINAL_SERVICE) == 0) {
		return new TerminalService(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* TerminalService::NDQueryInterface(const char* riid)
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
bool TerminalService::DoInitReport()
{
	m_reporter = (IReporter*)QI(CID_REPORTER, IID_REPORTER, "terminal service");
	if (!m_reporter) {
		LOG_ERR("Create reporter failed!");
		return false;
	}

	com::ReporterParam param;
	param.app = "default";
	param.space = "default";
	param.service_type = "terminal-service";
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
bool TerminalService::DoInitMq()
{
	m_amqp_client = (IAmqpClient*)QI(CID_AMQP_CLIENT, IID_AMQP_CLIENT,
		"terminal service");
	if (!m_amqp_client) {
		LOG_ERR("Create amqp client failed!");
		return false;
	}

	AmqpParam amqp_param;
	amqp_param.host = m_config.mq_config.addr.ep.host;
	amqp_param.port = m_config.mq_config.addr.ep.port;
	amqp_param.user = m_config.mq_config.user;
	amqp_param.pwd = m_config.mq_config.pwd;
	amqp_param.handler = this;
	amqp_param.owner = "terminal service";

	if (ERR_CODE_OK != m_amqp_client->Init(amqp_param)) {
		LOG_ERR("Initialize amqp client failed!");
		return false;
	}

	// Service exchagne
	if (ERR_CODE_OK != m_amqp_client->DeclareExchange(
		m_config.service_config.exchange, ExchangeType::FANOUT)) {
		LOG_ERR("Declare terminal exchange failed!");
		return false;
	}

	std::string service_queue = m_config.service_config.exchange + "_" +
		m_config.service_config.instance_id;

	// Service queue
	if (ERR_CODE_OK != m_amqp_client->DeclareQueue(service_queue)) {
		LOG_ERR("Declare terminal queue failed!");
		return false;
	}

	// Bind service queue to service exchange
	if (ERR_CODE_OK != m_amqp_client->BindQueue(m_config.service_config.exchange,
		service_queue, "")) {
		LOG_ERR("Bind queue failed!");
		return false;
	}

	// Notify exchagne
	if (ERR_CODE_OK != m_amqp_client->DeclareExchange(
		m_config.service_config.notify_exchange,
		ExchangeType::FANOUT)) {
		LOG_ERR("Declare terminal notify exchange failed!");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool TerminalService::Init(net::ISessionMgr* mgr, const std::string& config_file)
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

	m_msg_sender.reset(new MsgSender(m_amqp_client, m_config));

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool TerminalService::Start()
{
	StartThread();

	m_reporter->Start();

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TerminalService::Stop()
{
	m_reporter->Stop();

	StopThread();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool TerminalService::Reload()
{
	LOG_INF("Reload");

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TerminalService::OnServicePingMsg(const Buffer& mq_buf, const Buffer& sig_buf)
{
	m_msg_sender->SendServicePong(mq_buf, sig_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TerminalService::OnMqMsg(const CommonMsg& msg)
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
	case prot::MSG_CLIENT_REGISTER_REQ:
		OnRegisterReq(data->buf1, data->buf2);
		break;
	case prot::MSG_CLIENT_UNREGISTER_REQ:
		OnUnregisterReq(data->buf1, data->buf2);
		break;
	case prot::MSG_CLIENT_OFFLINE_REQ:
		OnClientOfflineReq(data->buf1, data->buf2);
		break;
	default:
		LOG_ERR("Unknown protocol type:{}", (uint32_t)sig_hdr->mt);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TerminalService::OnThreadMsg(const CommonMsg& msg)
{
	switch (msg.msg_type) {
	case TERMINAL_MSG_MQ_MSG:
		OnMqMsg(msg);
		break;
	default:
		LOG_ERR("Unknown messge type:{}", msg.msg_type);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TerminalService::OnRegisterReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	RegisterReqTupleOpt req_tuple = ParseRegisterReq(mq_buf, sig_buf);
	if (!req_tuple.has_value()) {
		LOG_ERR("Parse register request failed!");
		return; // TODO:
	}

	prot::RegisterReq reg_req = std::get<1>(req_tuple.value());

	LOG_INF("Received register request:{}", util::PbMsgToJson(reg_req));

	prot::RegisterReqExtendData ext_data = std::get<2>(req_tuple.value());

	for (const auto& item : m_terminals) {
		if (item.second.client_id == reg_req.client_id()) {
			LOG_ERR("Terminal entry exists");
			m_msg_sender->SendRegisterRsp(mq_buf, sig_buf, reg_req, 0,
				ERR_CODE_FAILED, "terminal entry exists");
			return; // TODO:
		}
	}

	TerminalEntry entry;
	entry.instance_id = ext_data.instance_id();
	entry.session_id = ext_data.session_id();
	entry.client_id = reg_req.client_id();
	entry.client_type = reg_req.client_type();
	entry.client_name = reg_req.client_name();
	entry.os = reg_req.os();
	entry.version = reg_req.version();
	entry.device = reg_req.device();
	entry.register_id = ++m_cur_reg_id;
	entry.register_time = util::Now();

	m_terminals.insert(std::make_pair(m_cur_reg_id, entry));

	m_msg_sender->SendRegisterRsp(mq_buf, sig_buf, reg_req, m_cur_reg_id,
		ERR_CODE_OK, "success");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TerminalService::OnUnregisterReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	UnregisterReqPairOpt pair = ParseUnregisterReq(mq_buf, sig_buf);
	if (!pair.has_value()) {
		LOG_ERR("Parse unregister request failed!");
		return; // TODO:
	}

	LOG_INF("Received unregister request:{}", util::PbMsgToJson(pair->second));

	auto iter = m_terminals.find(pair->second.register_id());
	if (iter == m_terminals.end()) {
		LOG_ERR("Cannot find terminal entry");
		m_msg_sender->SendUnregisterRsp(mq_buf, sig_buf, pair->second,
			ERR_CODE_FAILED, "cannot find terminal entry");
		return;
	}

	m_msg_sender->SendUnregisterRsp(mq_buf, sig_buf, pair->second, ERR_CODE_OK,
		"success");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void
TerminalService::OnClientOfflineReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	prot::ClientOfflineReq req;
	if (!req.ParseFromArray(PB_PARSE_SIG_PARAM(sig_buf))) {
		LOG_ERR("Parse client offline msg failed!");
		return;
	}

	LOG_INF("Received client offline request:{}", util::PbMsgToJson(req));

	auto it = m_terminals.begin();
	for (; it != m_terminals.end(); ++it) {
		if (it->second.instance_id == req.instance_id() &&
			it->second.session_id == req.session_id()) {
			break;
		}
	}

	if (it == m_terminals.end()) {
		LOG_ERR("Cannot find terminal entry");
		return; // TODO:
	}

	m_msg_sender->SendClientOfflineRsp(mq_buf, sig_buf, req);

	m_msg_sender->SendClientOfflineNotify(mq_buf, sig_buf, req, 
		it->second.register_id);

	m_terminals.erase(it);
}

//------------------------------------------------------------------------------
// Process in work thread
//------------------------------------------------------------------------------
void TerminalService::OnRecvMqMsg(const std::string& queue, const Buffer& mq_buf,
	const Buffer& sig_buf)
{
	std::shared_ptr<DoubleBuffer> mq_data(new DoubleBuffer(mq_buf, sig_buf));
	PostMsg(CommonMsg(TERMINAL_MSG_MQ_MSG, mq_data));
}

}