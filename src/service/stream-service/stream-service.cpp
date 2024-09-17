#include "stream-service.h"
#include "log.h"
#include "stream-common.h"
#include "protocol.h"
#include "sig-msg-builder.h"
#include "common/util-net.h"
#include "common/util-reporter.h"
#include "common/util-pb.h"
#include "httplib.h"
#include "msg-parser.h"
#include "util-protocol.h"

using namespace jukey::com;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
StreamService::StreamService(base::IComFactory* factory, const char* owner)
	: ProxyUnknown(nullptr)
	, ComObjTracer(factory, CID_STREAM_SERVICE, owner)
	, CommonThread("stream service", true)
	, m_factory(factory)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* StreamService::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_STREAM_SERVICE) == 0) {
		return new StreamService(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* StreamService::NDQueryInterface(const char* riid)
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
bool StreamService::DoInitMq()
{
	m_amqp_client = (IAmqpClient*)QI(CID_AMQP_CLIENT, IID_AMQP_CLIENT,
		"stream service");
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
	amqp_param.owner = "stream service";

	if (ERR_CODE_OK != m_amqp_client->Init(amqp_param)) {
		LOG_ERR("Initialize amqp client failed!");
		return false;
	}

	// Service exchagne
	if (ERR_CODE_OK != m_amqp_client->DeclareExchange(
		m_config.service_config.exchange, ExchangeType::FANOUT)) {
		LOG_ERR("Declare exchagne failed!");
		return false;
	}

	m_service_queue = m_config.service_config.exchange + "_" +
		m_config.service_config.instance_id;

	// Service queue
	if (ERR_CODE_OK != m_amqp_client->DeclareQueue(m_service_queue)) {
		LOG_ERR("Declare queue failed!");
		return false;
	}

	// Bind service queue to service exchange
	if (ERR_CODE_OK != m_amqp_client->BindQueue(
		m_config.service_config.exchange, m_service_queue, "")) {
		LOG_ERR("Bind queue failed!");
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool StreamService::DoInitReport()
{
	m_reporter = (IReporter*)QI(CID_REPORTER, IID_REPORTER, "stream service");
	if (!m_reporter) {
		LOG_ERR("Create reporter failed!");
		return false;
	}

	com::ReporterParam param;
	param.app = "default";
	param.space = "default";
	param.service_type = "stream-service";
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
bool StreamService::Init(net::ISessionMgr* mgr, const std::string& config_file)
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

	m_msg_sender.reset(new MsgSender(m_config, m_amqp_client, m_mq_async_proxy));

	LOG_INF("Init success")

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool StreamService::Start()
{
	StartThread();

	m_reporter->Start();

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamService::Stop()
{
	m_reporter->Stop();

	StopThread();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool StreamService::Reload()
{
	LOG_INF("Reload");

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamService::OnPubStreamReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	PubStreamReqPairOpt pair = ParsePubStreamReq(mq_buf, sig_buf);
	if (!pair.has_value()) {
		LOG_ERR("Parse publish stream request failed!");
		return; // TODO:
	}

	LOG_INF("Received publish stream request:{}", util::PbMsgToJson(pair->second));

	auto iter = m_streams.find(pair->second.stream().stream_id());
	if (iter != m_streams.end()) {
		m_msg_sender->SendPubStreamRsp(mq_buf, sig_buf, pair, ERR_CODE_FAILED,
			"stream exists");
		LOG_WRN("Stream exists, user:{}", iter->second.src.user_id);
	}
	else {
		m_streams.insert(std::make_pair(pair->second.stream().stream_id(),
			util::ToMediaStream(pair->second.stream())));

		m_msg_sender->SendPubStreamRsp(mq_buf, sig_buf, pair, ERR_CODE_OK,
			"success");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamService::OnUnpubStreamReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	UnpubStreamReqPairOpt pair = ParseUnpubStreamReq(mq_buf, sig_buf);
	if (!pair.has_value()) {
		LOG_ERR("Parse unpublish stream request failed!");
		return; // TODO:
	}

	LOG_INF("Received unpublish stream request:{}", util::PbMsgToJson(pair->second));

	auto iter = m_streams.find(pair->second.stream().stream_id());
	if (iter != m_streams.end()) {
		m_streams.erase(iter);
		m_msg_sender->SendUnpubStreamRsp(mq_buf, sig_buf, pair, ERR_CODE_OK,
			"success");
	}
	else {
		LOG_WRN("Cannot find Stream");
		m_msg_sender->SendUnpubStreamRsp(mq_buf, sig_buf, pair, ERR_CODE_FAILED,
			"failed");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamService::OnSubStreamReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	SubStreamReqPairOpt req_pair = ParseSubStreamReq(mq_buf, sig_buf);
	if (!req_pair.has_value()) {
		LOG_ERR("Parse subscribe stream request failed!");
		return; // TODO:
	}

	LOG_INF("Received subscribe stream request:{}", 
		util::PbMsgToJson(req_pair->second));

	auto iter = m_streams.find(req_pair->second.stream().stream_id());
	if (iter == m_streams.end()) {
		LOG_ERR("Cannot find stream");
		m_msg_sender->SendSubStreamRsp(mq_buf, sig_buf, req_pair, ERR_CODE_FAILED,
			"cannot find stream");
	}
	else {
		m_msg_sender->SendSubStreamRsp(mq_buf, sig_buf, req_pair, ERR_CODE_OK,
			"success");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamService::OnUnsubStreamReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	UnsubStreamReqPairOpt req_pair = ParseUnsubStreamReq(mq_buf, sig_buf);
	if (!req_pair.has_value()) {
		LOG_ERR("Parse unsubscribe stream request failed!");
		return; // TODO:
	}

	LOG_INF("Received unsubscribe stream request:{}",
		util::PbMsgToJson(req_pair->second));

	auto iter = m_streams.find(req_pair->second.stream().stream_id());
	if (iter == m_streams.end()) {
		LOG_ERR("Cannot find stream");
		m_msg_sender->SendUnsubStreamRsp(mq_buf, sig_buf, req_pair, ERR_CODE_FAILED,
			"cannot find stream");
	}
	else {
		m_msg_sender->SendUnsubStreamRsp(mq_buf, sig_buf, req_pair, ERR_CODE_OK,
			"success");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamService::OnServicePingMsg(const Buffer& mq_buf, const Buffer& sig_buf)
{
	ServicePingPairOpt ping_pair = ParseServicePing(mq_buf, sig_buf);
	if (!ping_pair.has_value()) {
		LOG_ERR("Parse service ping failed");
		return;
	}

	LOG_DBG("Received service ping:{}", util::PbMsgToJson(ping_pair->second));

	m_msg_sender->SendServicePong(mq_buf, sig_buf, ping_pair);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamService::OnGetParentNodeReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	prot::GetParentNodeReq req;
	if (!req.ParseFromArray(PB_PARSE_SIG_PARAM(sig_buf))) {
		LOG_ERR("Parse get parent node request failed!");
		return;
	}

	LOG_INF("Received get parent node request:{}", util::PbMsgToJson(req));

	auto iter = m_streams.find(req.stream().stream_id());
	if (iter == m_streams.end()) {
		LOG_ERR("Cannot find stream");
		m_msg_sender->SendGetParentNodeRsp(mq_buf, sig_buf, req, ERR_CODE_FAILED, 
			"cannot find stream");

	}
	else {
		// TODO: wait for client ack ???
		m_msg_sender->SendGetParentNodeRsp(mq_buf, sig_buf, req, ERR_CODE_OK, 
			"success");

		m_msg_sender->SendLoginSendChannelNotify(iter->second, req.service_addr(),
			++m_cur_seq);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamService::OnMqMsg(const CommonMsg& msg)
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
	case prot::MSG_PUBLISH_STREAM_REQ:
		OnPubStreamReq(data->buf1, data->buf2);
		break;
	case prot::MSG_UNPUBLISH_STREAM_REQ:
		OnUnpubStreamReq(data->buf1, data->buf2);
		break;
	case prot::MSG_SUBSCRIBE_STREAM_REQ:
		OnSubStreamReq(data->buf1, data->buf2);
		break;
	case prot::MSG_UNSUBSCRIBE_STREAM_REQ:
		OnUnsubStreamReq(data->buf1, data->buf2);
		break;
	case prot::MSG_GET_PARENT_NODE_REQ:
		OnGetParentNodeReq(data->buf1, data->buf2);
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
void StreamService::OnThreadMsg(const CommonMsg& msg)
{
	switch (msg.msg_type) {
	case STREAM_MSG_MQ_MSG:
		OnMqMsg(msg);
		break;
	default:
		LOG_ERR("Invalid message:{}", msg.msg_type);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamService::OnRecvMqMsg(const std::string& queue, const Buffer& mq_buf,
	const Buffer& sig_buf)
{
	std::shared_ptr<DoubleBuffer> mq_data(new DoubleBuffer(mq_buf, sig_buf));
	PostMsg(CommonMsg(STREAM_MSG_MQ_MSG, mq_data));
}

}