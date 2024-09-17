#include "transport-service.h"
#include "log.h"
#include "transport-common.h"
#include "protocol.h"
#include "net-message.h"
#include "protoc/mq.pb.h"
#include "protoc/transport.pb.h"
#include "transport-msg-builder.h"
#include "common/util-net.h"
#include "common/util-reporter.h"
#include "common/util-pb.h"
#include "yaml-cpp/yaml.h"
#include "httplib.h"
#include "msg-builder.h"
#include "util-protocol.h"

using namespace jukey::com;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
TransportService::TransportService(base::IComFactory* factory, const char* owner)
	: ProxyUnknown(nullptr)
	, ComObjTracer(factory, CID_TRANSPORT_SERVICE, owner)
	, CommonThread("transport service", 256, true)
	, m_factory(factory)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* TransportService::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_TRANSPORT_SERVICE) == 0) {
		return new TransportService(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* TransportService::NDQueryInterface(const char* riid)
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
bool TransportService::DoInitMq()
{
	m_amqp_client = (IAmqpClient*)QI(CID_AMQP_CLIENT, IID_AMQP_CLIENT,
		"transport service");
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
	amqp_param.owner   = "tranport service";

	if (ERR_CODE_OK != m_amqp_client->Init(amqp_param)) {
		LOG_ERR("Initialize amqp client failed!");
		return false;
	}

	// Declare service exchagne
	if (ERR_CODE_OK != m_amqp_client->DeclareExchange(
		m_config.service_config.exchange, ExchangeType::FANOUT)) {
		LOG_ERR("Declare exchagne failed!");
		return false;
	}

	std::string service_queue = m_config.service_config.exchange + "_" +
		m_config.service_config.instance_id;

	// Declare service queue
	if (ERR_CODE_OK != m_amqp_client->DeclareQueue(service_queue)) {
		LOG_ERR("Declare queue failed!");
		return false;
	}

	// Bind service queue to service exchange
	if (ERR_CODE_OK != m_amqp_client->BindQueue(m_config.service_config.exchange,
		service_queue, "")) {
		LOG_ERR("Bind queue failed!");
	}

	return true;
}

//------------------------------------------------------------------------------
// Listen on TCP port and UDP port
//------------------------------------------------------------------------------
bool TransportService::DoInitListen()
{
	net::ListenParam param;
	param.listen_addr = m_config.service_config.listen_addr;
	param.listen_srv = (ServiceType)m_config.service_config.service_type;
	param.thread = this;

	m_listen_id = m_sess_mgr->AddListen(param);
	if (m_listen_id == INVALID_LISTEN_ID) {
		LOG_ERR("Add tcp listen:{} failed!", "");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool TransportService::DoInitStreamExchange()
{
	m_stream_exch = (txp::IStreamExchange*)QI(CID_STREAM_EXCHAGE,
		IID_STREAM_EXCHAGE, "transport service");
	if (!m_stream_exch) {
		LOG_ERR("Create stream exchange failed!");
		return false;
	}

	if (ERR_CODE_OK != m_stream_exch->Init(this, this)) {
		LOG_ERR("Initialize stream exchage failed!");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool TransportService::DoInitReport()
{
	m_reporter = (IReporter*)QI(CID_REPORTER, IID_REPORTER, "transport service");
	if (!m_reporter) {
		LOG_ERR("Create reporter failed!");
		return false;
	}

	com::ReporterParam param;
	param.app = "default";
	param.space = "default";
	param.service_type = "transport-service";
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
void TransportService::DoInitStats()
{
	m_data_stats.reset(new util::DataStats(m_factory, g_logger, ""));
	m_data_stats->Start();

	util::StatsParam recv_stats("recv-br", util::StatsType::IAVER, 5000);
	m_recv_br_id = m_data_stats->AddStats(recv_stats);
	util::StatsParam send_stats("send-br", util::StatsType::IAVER, 5000);
	m_send_br_id = m_data_stats->AddStats(send_stats);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool TransportService::Init(net::ISessionMgr* mgr, const std::string& config_file)
{
	if (!mgr) {
		LOG_ERR("Invalid session manager!");
		return false;
	}
	m_sess_mgr = mgr;

	auto result = ParseConfig(config_file);
	if (!result.has_value()) {
		LOG_ERR("Parse config file:{} failed!", config_file);
		return false;
	}
	m_config = result.value();

	if (!DoInitListen()) {
		LOG_ERR("Initialize listen failed!");
		return false;
	}

	if (!DoInitMq()) {
		LOG_ERR("Initialize MQ failed!");
		return false;
	}

	if (!DoInitStreamExchange()) {
		LOG_ERR("Initialize stream exchange failed!");
		return false;
	}

	if (!DoInitReport()) {
		LOG_ERR("Initialize report failed!");
		return false;
	}

	DoInitStats();

	m_mq_async_proxy.reset(new util::MqAsyncProxy(
		m_factory, m_amqp_client, this, 10000));

	m_msg_sender.reset(new MsgSender(m_sess_mgr));

	LOG_INF("Init success");

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool TransportService::Start()
{
	StartThread();

	m_reporter->Start();

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TransportService::Stop()
{
	m_reporter->Stop();

	StopThread();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool TransportService::Reload()
{
	LOG_INF("Reload");

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TransportService::OnLoginSendChnlReq(net::SessionId sid, const Buffer& buf)
{
	prot::LoginSendChannelReq req;
	if (!req.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse login send channel request failed!");
		return;
	}

	LOG_INF("Received login send channel request:{}", util::PbMsgToJson(req));

	++m_chnl_id;

	LOG_INF("Alloc send channel:{}", m_chnl_id);

	com::MediaStream stream = util::ToMediaStream(req.stream());

	if (ERR_CODE_OK != m_stream_exch->AddSrcChannel(stream, m_chnl_id)) {
		LOG_ERR("Add send channel to exchange failed!");
		m_msg_sender->SendLoginSendChnlRsp(sid, m_chnl_id, buf, req,
			ERR_CODE_FAILED, "failed");
	}
	else {
		m_sess_chnl.insert(std::make_pair(sid, ChannelEntry(m_chnl_id, true, 
			stream.stream.stream_id)));
		LOG_INF("Add send channel to exchange success, session:{}, channel:{}", 
			sid, m_chnl_id);

		m_msg_sender->SendLoginSendChnlRsp(sid, m_chnl_id, buf, req, ERR_CODE_OK,
			"success");

		m_msg_sender->SendStartSendStreamNotify(++m_cur_seq, sid, m_chnl_id, buf, 
			req);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TransportService::OnLogoutSendChnlReq(net::SessionId sid,
	const Buffer& buf)
{
	prot::LogoutSendChannelReq req;
	if (!req.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse logout send channel request failed!");
		return;
	}

	LOG_INF("Received logout send channel request:{}", util::PbMsgToJson(req));

	com::MediaStream stream = util::ToMediaStream(req.stream());

	if (ERR_CODE_OK != m_stream_exch->RemoveSrcChannel(req.channel_id())) {
		LOG_ERR("Remove src channel from exchange failed");
		m_msg_sender->SendLogoutSendChnlRsp(sid, req.channel_id(), buf, req, 
			ERR_CODE_FAILED, "failed");
	}
	else {
		LOG_INF("Remove session channel, sid:{}, channel:{}", sid, m_chnl_id);
		m_sess_chnl.erase(sid);
		m_msg_sender->SendLogoutSendChnlRsp(sid, req.channel_id(), buf, req,
			ERR_CODE_OK, "success");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TransportService::GetParentStreamNode(net::SessionId sid,
	uint32_t channel_id, const Buffer& buf, const prot::LoginRecvChannelReq& req)
{
	MqMsgPair pair = BuildGetParentNodeReqPair(++m_cur_seq, req,
		m_config.service_config.listen_addr.ToStr(),
		m_config.service_config.exchange,
		m_config.service_config.service_type,
		m_config.service_config.instance_id);

	// Call stream service to publish stream
	m_mq_async_proxy->SendMqMsg("stream-exchange", "", pair.first,
		pair.second, m_cur_seq, prot::MSG_GET_PARENT_NODE_RSP)
		.OnResponse([this](const Buffer& mq_buf, const Buffer& prot_buf) {
			LOG_INF("Get parent node response");
			// TODO: no need to process on single server mode
		})
		.OnTimeout([this]() {
			LOG_ERR("Send get parent node request timeout");
		})
		.OnError([this](const std::string& err) {
			LOG_ERR("Send get parent node request error:{}", err);
		});

	LOG_INF("Send get parent node request to stream service, seq:{}, app:{}, "
		"user:{}, stream:{}|{}, addr:{}",
		m_cur_seq,
		req.app_id(),
		req.user_id(),
		req.stream().stream_type(),
		req.stream().stream_id(),
		m_config.service_config.listen_addr.ToStr());
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TransportService::OnLoginRecvChnlReq(net::SessionId sid, const Buffer& buf)
{
	prot::LoginRecvChannelReq req;
	if (!req.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse login recv channel request failed!");
		return;
	}

	LOG_INF("Received login recv channel request:{}", util::PbMsgToJson(req));

	++m_chnl_id;

	LOG_INF("Allocate recv channel:{}", m_chnl_id);

	com::MediaStream stream = util::ToMediaStream(req.stream());

	if (ERR_CODE_OK != m_stream_exch->AddDstChannel(stream, m_chnl_id, 
		req.user_id())) {
		LOG_ERR("Add recv channel to exchange failed!");
		m_msg_sender->SendLoginRecvChnlRsp(sid, m_chnl_id, buf, req,
			ERR_CODE_FAILED, "failed");
	}
	else {
		m_sess_chnl.insert(std::make_pair(sid, ChannelEntry(m_chnl_id, false,
			stream.stream.stream_id)));
		LOG_INF("Add recv channel to exchange success, session:{}, channel:{}",
			sid, m_chnl_id);

		m_msg_sender->SendLoginRecvChnlRsp(sid, m_chnl_id, buf, req, ERR_CODE_OK,
			"success");

		if (!m_stream_exch->HasSender(STRM_ID(stream))) {
			GetParentStreamNode(sid, m_chnl_id, buf, req);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TransportService::OnLogoutRecvChnlReq(net::SessionId sid, const Buffer& buf)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TransportService::OnStartSendStreamAck(net::SessionId sid, const Buffer& buf)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TransportService::OnChannelData(net::SessionId sid, const Buffer& buf)
{
	auto iter = m_sess_chnl.find(sid);
	if (iter == m_sess_chnl.end()) {
		LOG_ERR("Cannot find channel by session:{}", sid);
		return;
	}
	
	// 剥掉消息头（SigMsgHdr只在Service这一层看到）
	uint32_t buf_len = buf.data_len - sizeof(prot::SigMsgHdr);
	com::Buffer msg_buf(buf_len, buf_len);
	memcpy(DP(msg_buf), DP(buf) + sizeof(prot::SigMsgHdr), buf_len);

	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(buf);
	m_stream_exch->OnRecvChannelData(iter->second.channel_id, sig_hdr->mt, msg_buf);

	m_data_stats->OnData(m_recv_br_id, buf.data_len);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TransportService::OnChannelMsg(net::SessionId sid, const Buffer& buf)
{
	auto iter = m_sess_chnl.find(sid);
	if (iter == m_sess_chnl.end()) {
		LOG_ERR("Cannot find channel by session:{}", sid);
		return;
	}

	m_stream_exch->OnRecvChannelMsg(iter->second.channel_id, buf);

	m_data_stats->OnData(m_recv_br_id, buf.data_len);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TransportService::OnMqMsg(const CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(DoubleBuffer);

	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)(DP(data->buf1));
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)(DP(data->buf2));

	LOG_INF("Received mq message, mq msg:{}, sig msg:{}",
		prot::util::MSG_TYPE_STR(mq_hdr->mt),
		prot::util::MSG_TYPE_STR(sig_hdr->mt));

	prot::util::DumpMqHeader(g_logger, mq_hdr);
	prot::util::DumpSignalHeader(g_logger, sig_hdr);

	if (!m_mq_async_proxy->OnMqMsg(data->buf1, data->buf2)) {
		LOG_ERR("Unknown message type:{}", msg.msg_type);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TransportService::OnSessionClosed(const CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(net::SessionClosedMsg);

	auto iter = m_sess_chnl.find(data->lsid);
	if (iter == m_sess_chnl.end()) {
		LOG_ERR("Cannot find closed session:{}", data->lsid);
		return;
	}

	if (iter->second.send) {
		m_stream_exch->RemoveSrcChannel(iter->second.channel_id);
	}
	else {
		m_stream_exch->RemoveDstChannel(iter->second.channel_id, 
			iter->second.stream_id);
	}

	LOG_INF("Session:{} closed, remove it, channel:{}, send:{}, stream:{}",
		data->lsid, 
		iter->second.channel_id, 
		iter->second.send, 
		iter->second.stream_id);

	m_sess_chnl.erase(iter);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TransportService::OnSessionCreateResult(const CommonMsg& msg)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// Receive message from session, send to service exchange 
//------------------------------------------------------------------------------
void TransportService::OnSessionData(const CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(net::SessionDataMsg);

	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(data->buf);

	if (sig_hdr->mt != prot::MSG_STREAM_DATA) {
		LOG_INF("Recevied session data, sid:{}, msg:{}, len:{}, start:{}",
			data->lsid,
			prot::util::MSG_TYPE_STR(sig_hdr->mt),
			data->buf.data_len,
			data->buf.start_pos);

		prot::util::DumpSignalHeader(g_logger, sig_hdr);
	}

	switch (sig_hdr->mt) {
	case prot::MSG_LOGIN_SEND_CHANNEL_REQ:
		OnLoginSendChnlReq(data->lsid, data->buf);
		break;
	case prot::MSG_LOGOUT_SEND_CHANNEL_REQ:
		OnLogoutSendChnlReq(data->lsid, data->buf);
		break;
	case prot::MSG_LOGIN_RECV_CHANNEL_REQ:
		OnLoginRecvChnlReq(data->lsid, data->buf);
		break;
	case prot::MSG_LOGOUT_RECV_CHANNEL_REQ:
		OnLogoutRecvChnlReq(data->lsid, data->buf);
		break;
	case prot::MSG_START_SEND_STREAM_ACK:
		OnStartSendStreamAck(data->lsid, data->buf);
		break;
	case prot::MSG_STOP_SEND_STREAM_ACK:
		break;
	case prot::MSG_PAUSE_RECV_STREAM_REQ:
		break;
	case prot::MSG_RESUME_RECV_STREAM_REQ:
		break;
	case prot::MSG_NEGOTIATE_REQ:
	case prot::MSG_NEGOTIATE_RSP:
		OnChannelMsg(data->lsid, data->buf);
		break;
	case prot::MSG_STREAM_DATA:
	case prot::MSG_STREAM_FEEDBACK:
		OnChannelData(data->lsid, data->buf);
		break;
	default:
		LOG_ERR("Unknown protocol:{}", (uint32_t)sig_hdr->mt);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TransportService::OnSessionIncomming(const CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(net::SessionIncommingMsg);

	LOG_INF("Incoming session, lsid:{}, rsid:{}, addr:{}", data->lsid, data->rsid,
		data->addr.ToStr());
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TransportService::OnThreadMsg(const CommonMsg& msg)
{
	switch (msg.msg_type) {
	case TRANSPORT_MSG_MQ_MSG:
		OnMqMsg(msg);
		break;
	case net::NET_MSG_SESSION_CLOSED:
		OnSessionClosed(msg);
		break;
	case net::NET_MSG_SESSION_CREATE_RESULT:
		OnSessionCreateResult(msg);
		break;
	case net::NET_MSG_SESSION_DATA:
		OnSessionData(msg);
		break;
	case net::NET_MSG_SESSION_INCOMING:
		OnSessionIncomming(msg);
		break;
	default:
		LOG_ERR("Unknown message type:{}", msg.msg_type);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TransportService::OnRecvMqMsg(const std::string& queue,
	const com::Buffer& mq_buf, const com::Buffer& sig_buf)
{
	LOG_DBG("Receve message from queue:{}", queue);

	std::shared_ptr<DoubleBuffer> mq_data(new DoubleBuffer(mq_buf, sig_buf));
	PostMsg(CommonMsg(TRANSPORT_MSG_MQ_MSG, mq_data));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TransportService::OnSendChannelMsg(uint32_t channel_id, uint32_t user_id,
	const Buffer& buf)
{
	// TODO: performance
	net::SessionId session_id = INVALID_SESSION_ID;
	for (auto item : m_sess_chnl) {
		if (item.second.channel_id == channel_id) {
			session_id = item.first;
			break;
		}
	}

	if (session_id == INVALID_SESSION_ID) {
		LOG_ERR("Cannot find session by channel:{}", channel_id);
		return;
	}
	
	if (ERR_CODE_OK != m_sess_mgr->SendData(session_id, buf)) {
		LOG_ERR("Send msg to channel:{} failed!", channel_id);
	}

	m_data_stats->OnData(m_send_br_id, buf.data_len);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TransportService::OnSendChannelData(uint32_t channel_id, uint32_t user_id,
	uint32_t mt, const Buffer& buf)
{
	// TODO: performance
	net::SessionId session_id = INVALID_SESSION_ID;
	for (auto item : m_sess_chnl) {
		if (item.second.channel_id == channel_id) {
			session_id = item.first;
			break;
		}
	}

	if (session_id == INVALID_SESSION_ID) {
		LOG_ERR("Cannot find session by channel:{}", channel_id);
		return;
	}

	// 添加 SigMsgHdr
	uint32_t buf_len = buf.data_len + sizeof(prot::SigMsgHdr);
	Buffer sig_buf(buf_len, buf_len);

	// Transport service 的 signal 消息不经过 router 和 proxy 转发，
	// 因此 SigMsgHdr 只需要设置必要的几个字段即可
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);
	sig_hdr->len = buf.data_len;
	sig_hdr->mt = mt;
	sig_hdr->seq = ++m_cur_seq; // TODO: 每个 channel 的序列号独立？
	sig_hdr->usr = user_id;
	memcpy(DP(sig_buf) + sizeof(prot::SigMsgHdr), DP(buf), buf.data_len);

	if (ERR_CODE_OK != m_sess_mgr->SendData(session_id, sig_buf)) {
		LOG_ERR("Send data to channel:{} failed!", channel_id);
	}

	m_data_stats->OnData(m_send_br_id, buf.data_len);
}

}