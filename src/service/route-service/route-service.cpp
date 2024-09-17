#include "route-service.h"
#include "log.h"
#include "common/util-net.h"
#include "common/util-time.h"
#include "common/util-common.h"
#include "common/util-reporter.h"
#include "net-message.h"
#include "protoc/mq.pb.h"
#include "httplib.h"
#include "protoc/topo.pb.h"
#include "topo-msg-builder.h"
#include "util-protocol.h"
#include "route-common.h"

using namespace jukey::com;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
RouteService::RouteService(base::IComFactory* factory, const char* owner)
	: ProxyUnknown(nullptr)
	, ComObjTracer(factory, CID_ROUTE_SERVICE, owner)
	, CommonThread("route service", true)
	, m_factory(factory)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* RouteService::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_ROUTE_SERVICE) == 0) {
		return new RouteService(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* RouteService::NDQueryInterface(const char* riid)
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
bool RouteService::ContructRouteMap()
{
	for (auto route : m_config.routes) {
		for (auto msg : route.messages) {
			if (m_service_routes.find(msg) != m_service_routes.end()) {
				LOG_ERR("Repeat message:{}", msg);
				return false;
			}
			m_service_routes.insert(std::make_pair(msg, route.exchange));
		}
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool RouteService::DoInitListen()
{
	net::ListenParam param;
	param.listen_addr = m_config.service_config.listen_addr;
	param.listen_srv = (ServiceType)m_config.service_config.service_type;
	param.thread = this;

	m_listen_id = m_sess_mgr->AddListen(param);
	if (m_listen_id == INVALID_LISTEN_ID) {
		LOG_ERR("Listen on {} failed!", m_config.service_config.listen_addr.ToStr());
		return false;
	}

	LOG_INF("Start listening on {}", param.listen_addr.ToStr());

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool RouteService::DoInitSerivceMq()
{
	// Service exchagne
	if (ERR_CODE_OK != m_amqp_client->DeclareExchange(
		m_config.service_config.exchange, ExchangeType::FANOUT)) {
		LOG_ERR("Declare exchange:{} failed!", m_config.service_config.exchange);
		return false;
	}

	m_service_queue = m_config.service_config.exchange + "_" +
		m_config.service_config.instance_id;

	// Service queue
	if (ERR_CODE_OK != m_amqp_client->DeclareQueue(m_service_queue)) {
		LOG_ERR("Declare queue:{} failed!", m_service_queue);
		return false;
	}

	// Bind service queue to service exchange
	if (ERR_CODE_OK != m_amqp_client->BindQueue(
		m_config.service_config.exchange, m_service_queue, "")) {
		LOG_ERR("Bind queue:{} to exchange:{} failed!", m_service_queue,
			m_config.service_config.exchange);
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool RouteService::DoInitPongMq()
{
	// Ping exchagne
	if (ERR_CODE_OK != m_amqp_client->DeclareExchange(
		m_config.ping_config.exchange, ExchangeType::DIRECT)) {
		LOG_ERR("Declare exchange:{} failed!", m_config.ping_config.exchange);
		return false;
	}

	m_ping_queue = m_config.ping_config.exchange + "_" +
		m_config.service_config.instance_id;

	// Ping queue
	if (ERR_CODE_OK != m_amqp_client->DeclareQueue(m_ping_queue)) {
		LOG_ERR("Declare queue:{} failed!", m_ping_queue);
		return false;
	}

	// Bind ping queue to ping exchange
	if (ERR_CODE_OK != m_amqp_client->BindQueue(m_config.ping_config.exchange,
		m_ping_queue, m_config.service_config.instance_id)) {
		LOG_ERR("Bind queue:{} to exchange:{} failed!", m_ping_queue,
			m_config.ping_config.exchange);
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool RouteService::DoInitMq()
{
	m_amqp_client = (IAmqpClient*)QI(CID_AMQP_CLIENT, IID_AMQP_CLIENT,
		"route service");
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
	amqp_param.owner = "route service";

	if (ERR_CODE_OK != m_amqp_client->Init(amqp_param)) {
		LOG_ERR("Initialize amqp client failed!");
		return false;
	}

	if (!DoInitSerivceMq() || !DoInitPongMq()) {
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool RouteService::DoInitReport()
{
	m_reporter = (IReporter*)QI(CID_REPORTER, IID_REPORTER, "route service");
	if (!m_reporter) {
		LOG_ERR("Create reporter failed!");
		return false;
	}

	com::ReporterParam param;
	param.app = "default";
	param.space = "default";
	param.service_type = "route-service";
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

	for (auto route : m_config.routes) {
		m_reporter->AddReportEntry(DependEntry{
			route.service_name,
			route.service_name,
			"mq",
			route.service_name,
			"*",
			1 });
	}

	m_reporter->AddReportEntry(DependEntry{
		"mq",
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
bool RouteService::DoInitPinger()
{
	m_pinger = (IPinger*)QI(CID_PINGER, IID_PINGER, "route service");
	if (!m_pinger) {
		LOG_ERR("Create pinger failed!");
		return false;
	}

	com::ServiceParam param;
	param.service_name = m_config.service_config.service_name;
	param.service_type = m_config.service_config.service_type;
	param.instance_id = m_config.service_config.instance_id;

	if (!m_pinger->Init(param, this, m_config.ping_config.interval)) {
		LOG_ERR("Init pinger failed");
		return false;
	}

	for (auto& service : m_config.routes) {
		com::ServiceParam param;
		param.service_type = service.service_type;
		param.service_name = service.service_name;
		param.instance_id = "";

		m_pinger->AddPingService(param);
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool RouteService::Init(net::ISessionMgr* mgr, const std::string& config_file)
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

	if (!ContructRouteMap()) {
		LOG_ERR("Construct route map failed!");
		return false;
	}

	if (!DoInitListen()) {
		LOG_ERR("Listen failed!");
		return false;
	}

	if (!DoInitMq()) {
		LOG_ERR("Initialize MQ failed!");
		return false;
	}

	if (!DoInitReport()) {
		LOG_ERR("Initialize report failed!");
		return false;
	}

	if (!DoInitPinger()) {
		LOG_ERR("Initialize pinger failed!");
		return false;
	}

	m_mq_msg_sender.Init(m_amqp_client,
		m_config.service_config.service_type,
		m_config.service_config.instance_id,
		m_config.service_config.exchange,
		""/*routing key*/);

	LOG_INF("Init success");

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool RouteService::Start()
{
	StartThread();

	m_reporter->Start();

	m_pinger->Start();

	LOG_INF("Start route service");

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::Stop()
{
	m_pinger->Stop();

	m_reporter->Stop();

	StopThread();

	LOG_INF("Stop route service");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool RouteService::Reload()
{
	LOG_INF("Reload");

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::LearnAppRouteEntry(net::SessionId sid, prot::SigMsgHdr* sig_hdr)
{
	auto app_iter = m_app_route_tbl.find(sig_hdr->app);
	if (app_iter != m_app_route_tbl.end()) {
		auto entry_iter = app_iter->second.find(sid);
		if (entry_iter == app_iter->second.end()) {
			LOG_INF("Add app route session, app:{}, sid:{}", sig_hdr->app, sid);
			app_iter->second.insert(std::make_pair(sid, util::Now()));
		}
		else {
			LOG_DBG("Update app route session, app:{}, client:{}", sig_hdr->app,
				sig_hdr->clt);
			entry_iter->second = util::Now();
		}
	}
	else {
		RouteMap route_map;
		route_map.insert(std::make_pair(sid, util::Now()));
		m_app_route_tbl.insert(std::make_pair(sig_hdr->app, route_map));
		LOG_INF("Add app route entry, app:{}, sid:{}", sig_hdr->app, sid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::LearnClientRouteEntry(net::SessionId sid, prot::SigMsgHdr* sig_hdr)
{
	if (sig_hdr->clt == 0) {
		return;
	}

	uint64_t client_key = util::CombineTwoInt(sig_hdr->app, sig_hdr->clt);
	auto client_iter = m_client_route_tbl.find(client_key);
	if (client_iter != m_client_route_tbl.end()) {
		if (client_iter->second.sid != sid) {
			client_iter->second.sid = sid;
			LOG_WRN("Change client route session:{} to {}, key:{}|{}",
				client_iter->second.sid, sid, sig_hdr->app, sig_hdr->clt);
		}
		client_iter->second.last_update = util::Now();
		LOG_DBG("Update client route session, key:{}|{}, sid:{}",
			sig_hdr->app, sig_hdr->clt, sid);
	}
	else {
		m_client_route_tbl.insert(std::make_pair(client_key,
			RouteEntry(sid, util::Now())));
		LOG_INF("Add client route entry, key:{}|{}, sid:{}", sig_hdr->app,
			sig_hdr->clt, sid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::LearnUserRouteEntry(net::SessionId sid, prot::SigMsgHdr* sig_hdr)
{
	if (sig_hdr->usr == 0) {
		return;
	}

	uint64_t user_key = util::CombineTwoInt(sig_hdr->app, sig_hdr->usr);
	auto user_iter = m_user_route_tbl.find(user_key);
	if (user_iter != m_user_route_tbl.end()) {
		if (user_iter->second.sid != sid) {
			user_iter->second.sid = sid;
			LOG_WRN("Change user route session:{} to {}, key:{}|{}",
				user_iter->second.sid, sid, sig_hdr->app, sig_hdr->usr);
		}
		user_iter->second.last_update = util::Now();
		LOG_DBG("Update user route session, key:{}|{}, sid:{}",
			sig_hdr->app, sig_hdr->usr, sid);
	}
	else {
		m_user_route_tbl.insert(std::make_pair(user_key,
			RouteEntry(sid, util::Now())));
		LOG_INF("Add user route entry, key:{}|{}, sid:{}", sig_hdr->app,
			sig_hdr->usr, sid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::LearnGroupRouteEntry(net::SessionId sid, prot::SigMsgHdr* sig_hdr)
{
	if (sig_hdr->grp == 0) {
		return;
	}

	uint64_t group_key = util::CombineTwoInt(sig_hdr->app, sig_hdr->grp);
	auto group_iter = m_group_route_tbl.find(group_key);
	if (group_iter != m_group_route_tbl.end()) {
		auto entry_iter = group_iter->second.find(sid);
		if (entry_iter == group_iter->second.end()) {
			group_iter->second.insert(std::make_pair(sid, util::Now()));
			LOG_INF("Add group route session, key:{}|{}, sid:{}",
				sig_hdr->app, sig_hdr->grp, sid);
		}
		else {
			entry_iter->second = util::Now();
			LOG_DBG("Update group route session, key:{}|{}, sid:{}",
				sig_hdr->app, sig_hdr->grp, sid);
		}
	}
	else {
		RouteMap route_map;
		route_map.insert(std::make_pair(sid, util::Now()));
		m_group_route_tbl.insert(std::make_pair(group_key, route_map));
		LOG_INF("Add group route entry, key:{}|{}, sid:{}",
			sig_hdr->app, sig_hdr->grp, sid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::LearnRouteEntry(net::SessionId sid, prot::SigMsgHdr* sig_hdr)
{
	if (sig_hdr->app == 0) {
		LOG_ERR("Invalid app ID!");
		return;
	}

	// Find proxy service
	auto srv_iter = m_proxy_services.find(sid);
	if (srv_iter == m_proxy_services.end()) {
		LOG_ERR("Cannot find proxy serivce by session:{}", sid);
		return;
	}

	// App
	LearnAppRouteEntry(sid, sig_hdr);

	// Client
	LearnClientRouteEntry(sid, sig_hdr);

	// User
	LearnUserRouteEntry(sid, sig_hdr);

	// Group
	LearnGroupRouteEntry(sid, sig_hdr);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::SendMsgToService(const Buffer& buf)
{
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(buf);

	auto iter = m_service_routes.find(sig_hdr->mt);
	if (iter == m_service_routes.end()) {
		LOG_ERR("Cannot find service route for message:{}", (uint16_t)sig_hdr->mt);
		return;
	}

	m_mq_msg_sender.SendMqMsg(iter->second, "", buf, "");

	LOG_INF("Send message to service, exchange:{}, msg:{}, len:{}",
		iter->second, prot::util::MSG_TYPE_STR(sig_hdr->mt), buf.data_len);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::OnSessionClosed(const CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(net::SessionClosedMsg);

	LOG_INF("Session closed, lsid:{}, rsid:{}", data->lsid, data->rsid);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::OnSessionCreateResult(const CommonMsg& msg)
{
	LOG_INF("{}", __FUNCTION__);

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::OnServicePingMsg(net::SessionId sid, const Buffer& buf)
{
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)buf.data.get();

	prot::Ping ping_msg;
	if (!ping_msg.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse signal message failed!");
		return;
	}

	LOG_INF("Received service ping, service type:{}, service name:{}, instance:{}",
		ping_msg.service_type(), ping_msg.service_name(), ping_msg.instance_id());

	Buffer send_sig_buf = prot::util::BuildPongMsg(sig_hdr->seq,
		m_config.service_config.service_name,
		m_config.service_config.service_type,
		m_config.service_config.instance_id);

	if (ERR_CODE_OK != m_sess_mgr->SendData(sid, send_sig_buf)) {
		LOG_ERR("Send sessiond data failed");
	}
}

//------------------------------------------------------------------------------
// Receive message from session, send to service exchange 
//------------------------------------------------------------------------------
void RouteService::OnSessionData(const CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(net::SessionDataMsg);

	// Find proxy service
	auto iter = m_proxy_services.find(data->lsid);
	if (iter == m_proxy_services.end()) {
		LOG_ERR("Cannot find proxy service by session:{}", data->lsid);
		return;
	}

	// Parser message
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(data->buf);

	LOG_INF("Recevied session data, sid:{}, msg:{}, len:{}",
		data->lsid, prot::util::MSG_TYPE_STR(sig_hdr->mt), data->buf.data_len);

	prot::util::DumpSignalHeader(g_logger, sig_hdr);

	if (sig_hdr->mt == prot::MSG_SERVICE_PING) {
		OnServicePingMsg(data->lsid, data->buf);
	}
	else {
		LearnRouteEntry(data->lsid, sig_hdr);
		
		//SendMsgToService(Buffer((char*)sig_hdr, sizeof(prot::SigMsgHdr) + sig_hdr->len));
		SendMsgToService(data->buf);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::OnSessionIncomming(const CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(net::SessionIncommingMsg);

	LOG_INF("Incoming session, lsid:{}, rsid:{}, addr:{}", data->lsid, data->rsid,
		data->addr.ToStr());

	auto iter = m_proxy_services.find(data->lsid);
	if (iter != m_proxy_services.end()) {
		LOG_ERR("Session:{} already exists!", data->lsid);
		m_proxy_services.erase(iter);
	}

	ProxyRouteInfoSP pri(new ProxyRouteInfo);
	pri->remote_addr = data->addr;

	m_proxy_services.insert(std::make_pair(data->lsid, pri));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::OnMqMsg(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(DoubleBuffer);

	const com::Buffer& mq_buf  = data->buf1;
	const com::Buffer& sig_buf = data->buf2;

	prot::MqMsgHdr* mq_hdr   = (prot::MqMsgHdr*)DP(mq_buf);
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	LOG_INF("Received mq message, mq msg:{}, sig msg:{}",
		prot::util::MSG_TYPE_STR(mq_hdr->mt),
		prot::util::MSG_TYPE_STR(sig_hdr->mt));

	prot::util::DumpMqHeader(g_logger, mq_hdr);
	prot::util::DumpSignalHeader(g_logger, sig_hdr);

	// Parse MQ message
	prot::MqMsg mq_msg;
	if (!mq_msg.ParseFromArray(PB_PARSE_MQ_PARAM(mq_buf))) {
		LOG_ERR("Parse to proxy mq msg failed!");
		return;
	}

	if (sig_hdr->mt == prot::MSG_SERVICE_PONG) {
		if (m_pinger) {
			m_pinger->OnRecvPongMsg(sig_buf);
		}
	}
	else {
		SendMsgToProxy(sig_buf);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::OnSendPing(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(SendPingData);

	for (auto& service : m_config.routes) {
		if (service.service_type == data->param.service_type) {
			m_mq_msg_sender.SendMqMsg(service.exchange,
				"", // routing key
				data->buf,
				"", // user data
				m_config.ping_config.exchange,
				m_config.service_config.instance_id);
			LOG_INF("Send service ping to service, exchange:{}", service.exchange);
			break;
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::OnPingResult(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(PingResultData);

	bool found = false;
	for (auto& service : m_config.routes) {
		if (service.service_type == data->param.service_type) {
			m_reporter->UpdateReportEntry(service.service_name,
				(uint32_t)(data->result ? 0 : 1));
			if (!data->result) {
				LOG_WRN("ping failed, type:{}, name:{}, instance:{}",
					data->param.service_type,
					data->param.service_name,
					data->param.instance_id);
			}
			found = true;
			break;
		}
	}

	if (!found) {
		LOG_WRN("cannot find service for ping result, service type:{}, "
			"service name:{}, instance:{}, result:{}",
			data->param.service_type,
			data->param.service_name,
			data->param.instance_id,
			data->result);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::OnThreadMsg(const CommonMsg& msg)
{
	switch (msg.msg_type) {
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
	case ROUTE_MSG_MQ_MSG:
		OnMqMsg(msg);
		break;
	case ROUTE_MSG_SEND_PING:
		OnSendPing(msg);
		break;
	case ROUTE_MSG_PING_RESULT:
		OnPingResult(msg);
		break;
	default:
		LOG_ERR("Unknown message type:{}", msg.msg_type);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::SendMulticastMsgToProxy(const Buffer& sig_buf)
{
	prot::SigMsgHdr* header = (prot::SigMsgHdr*)sig_buf.data.get();

	uint64_t route_key = util::CombineTwoInt(header->app, header->grp);

	auto iter = m_group_route_tbl.find(route_key);
	if (iter == m_group_route_tbl.end()) {
		LOG_WRN("Cannot find group route:{}|{}", header->app, header->grp);
		return; // TODO: broadcast
	}

	// Send message to all proxies that have learned the group route entry
	for (auto& item : iter->second) {
		LOG_INF("Send multicast message to proxy, sid:{}, msg:{} len:{}",
			item.first, prot::util::MSG_TYPE_STR(header->mt), sig_buf.data_len);

		ErrCode result = m_sess_mgr->SendData(item.first, sig_buf);
		if (ERR_CODE_OK != result) {
			LOG_ERR("Send multicast message to proxy failed");
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::SendClientUnicastMsgToProxy(const Buffer& sig_buf)
{
	prot::SigMsgHdr* header = (prot::SigMsgHdr*)sig_buf.data.get();

	uint64_t route_key = util::CombineTwoInt(header->app, header->clt);

	auto iter = m_client_route_tbl.find(route_key);
	if (iter == m_client_route_tbl.end()) {
		LOG_ERR("Cannot find user route entry by key:{}|{}", header->app,
			header->clt);
		return; // TODO: broadcast
	}

	LOG_INF("Send client unicast message to proxy, sid:{}, msg:{}, len:{}",
		iter->second.sid, (uint32_t)header->mt, sig_buf.data_len);

	// Send message to client
	ErrCode result = m_sess_mgr->SendData(iter->second.sid, sig_buf);
	if (ERR_CODE_OK != result) {
		LOG_ERR("Send client unicast msg to proxy failed");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::SendUserUnicastMsgToProxy(const Buffer& sig_buf)
{
	prot::SigMsgHdr* header = (prot::SigMsgHdr*)DP(sig_buf);

	uint64_t route_key = util::CombineTwoInt(header->app, header->usr);

	auto iter = m_user_route_tbl.find(route_key);
	if (iter == m_user_route_tbl.end()) {
		LOG_ERR("Cannot find user route entry:{}|{}", header->app, header->usr);
		return;
	}

	LOG_INF("Send user unicast message to proxy, sid:{}, msg:{}, len:{}",
		iter->second.sid, prot::util::MSG_TYPE_STR(header->mt), sig_buf.data_len);

	// Send message to client
	ErrCode result = m_sess_mgr->SendData(iter->second.sid, sig_buf);
	if (ERR_CODE_OK != result) {
		LOG_ERR("Send user unicast message to proxy failed");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::SendBroadcastMsgToProxy(const Buffer& sig_buf)
{
	LOG_INF("{}", __FUNCTION__);

	// TODO
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::SendMsgToProxy(const Buffer& sig_buf)
{
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)sig_buf.data.get();

	if (sig_hdr->app == 0) {
		LOG_ERR("Invalid app ID!");
		return;
	}

	if (sig_hdr->grp != 0) {
		if (sig_hdr->usr != 0) {
			SendUserUnicastMsgToProxy(sig_buf);
		}
		else {
			SendMulticastMsgToProxy(sig_buf);
		}
	}
	else { // group == 0
		if (sig_hdr->usr != 0) {
			SendUserUnicastMsgToProxy(sig_buf);
		}
		else {
			if (sig_hdr->clt != 0) {
				SendClientUnicastMsgToProxy(sig_buf);
			}
			else {
				LOG_WRN("All flags are not set");
				SendBroadcastMsgToProxy(sig_buf);
			}
		}
	}
}

//------------------------------------------------------------------------------
// Receive message from message queue
//------------------------------------------------------------------------------
void RouteService::OnRecvMqMsg(const std::string& queue, const Buffer& mq_buf,
	const Buffer& sig_buf)
{
	std::shared_ptr<DoubleBuffer> mq_data(new DoubleBuffer(mq_buf, sig_buf));
	PostMsg(CommonMsg(ROUTE_MSG_MQ_MSG, mq_data));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::SendPing(const com::ServiceParam& param, const Buffer& buf)
{
	SendPingDataSP data(new SendPingData(param, buf));
	PostMsg(CommonMsg(ROUTE_MSG_SEND_PING, data));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteService::OnPingResult(const com::ServiceParam& param, bool result)
{
	PingResultDataSP data(new PingResultData(param, result));
	PostMsg(CommonMsg(ROUTE_MSG_PING_RESULT, data));
}

}