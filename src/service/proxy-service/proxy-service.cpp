#include "proxy-service.h"
#include "log.h"
#include "common/util-net.h"
#include "common/util-common.h"
#include "common/util-reporter.h"
#include "net-message.h"
#include "protoc/mq.pb.h"
#include "service-msg-processor.h"
#include "client-msg-processor.h"
#include "httplib.h"
#include "protocol.h"
#include "terminal-msg-builder.h"
#include "util-protocol.h"

using namespace jukey::com;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ProxyService::ProxyService(base::IComFactory* factory, const char* owner)
	: ProxyUnknown(nullptr)
	, ComObjTracer(factory, CID_PROXY_SERVICE, owner)
	, CommonThread("proxy service", true)
	, m_factory(factory)
{
	m_route_entry_mgr.reset(new RouteEntryMgr());
	m_route_service_mgr.reset(new RouteServiceMgr());
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* ProxyService::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_PROXY_SERVICE) == 0) {
		return new ProxyService(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* ProxyService::NDQueryInterface(const char* riid)
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
bool ProxyService::DoListen()
{
	net::ListenParam param;
	param.listen_addr = m_config.service_config.listen_addr;
	param.listen_srv = (ServiceType)m_config.service_config.service_type;
	param.thread = this;

	m_listen_id = m_sess_mgr->AddListen(param);
	if (m_listen_id == INVALID_LISTEN_ID) {
		LOG_ERR("Proxy service listen on address:{} failed!",
			m_config.service_config.listen_addr.ToStr());
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// TODO: wait for connecting result sync
//------------------------------------------------------------------------------
bool ProxyService::ConnectRouteServices()
{
	for (auto& service : m_config.route_services) {
		net::CreateParam param;
		param.remote_addr = service.service_addr;
		param.ka_interval = 5; // second
		param.service_type = ServiceType::ROUTE;
		param.session_type = net::SessionType::RELIABLE;
		param.thread = this;

		net::SessionId sid = m_sess_mgr->CreateSession(param);
		if (sid == INVALID_SESSION_ID) {
			LOG_ERR("Create session failed, addr:{}", service.service_addr.ToStr());
			return false;
		}

		LOG_INF("Connecting to route sevice, service type:{}, addr:{}, sid:{}",
			param.service_type, service.service_addr.ToStr(), sid);
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ProxyService::DoInitReport()
{
	m_reporter = (IReporter*)QI(CID_REPORTER, IID_REPORTER, "proxy service");
	if (!m_reporter) {
		LOG_ERR("Create reporter failed!");
		return false;
	}

	com::ReporterParam param;
	param.app = "default";
	param.space = "default";
	param.service_type = "proxy-service";
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

	for (auto& service : m_config.route_services) {
		m_reporter->AddReportEntry(DependEntry{
			service.instance_id,
			"route service",
			"session",
			service.service_name,
			service.instance_id,
			1 });
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ProxyService::DoInitPinger()
{
	m_pinger = (IPinger*)QI(CID_PINGER, IID_PINGER, "proxy service");
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

	for (auto service : m_config.route_services) {
		com::ServiceParam param;
		param.service_type = service.service_type;
		param.service_name = service.service_name;
		param.instance_id = service.instance_id;

		m_pinger->AddPingService(param);
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ProxyService::Init(net::ISessionMgr* mgr, const std::string& config_file)
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

	if (!DoListen()) {
		LOG_ERR("Listen failed!");
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

	if (!ConnectRouteServices()) {
		LOG_ERR("Connect route services failed!");
		return false;
	}

	m_client_msg_processor.Init(m_config,
		m_sess_mgr,
		m_route_entry_mgr,
		m_route_service_mgr);

	m_service_msg_processor.Init(m_sess_mgr, m_route_entry_mgr);

	LOG_INF("Initialize proxy service success");

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ProxyService::Start()
{
	StartThread();

	m_reporter->Start();

	m_pinger->Start();

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ProxyService::Stop()
{
	m_pinger->Stop();

	m_reporter->Stop();

	StopThread();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ProxyService::Reload()
{
	LOG_INF("Reload");

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ProxyService::SendMsgToRouteService(const com::Buffer& buf)
{
	std::optional<RouteServiceEntry> service_entry =
		m_route_service_mgr->GetAvaliableRouteServiceEntry();
	if (!service_entry.has_value()) {
		LOG_ERR("Cannot get avaliable route service!");
		return;
	}

	prot::SigMsgHdr* hdr = (prot::SigMsgHdr*)DP(buf);

	LOG_INF("Send message to route service, addr:{}, sid:{}, msg:{}, len:{}",
		service_entry->service_addr.ToStr(),
		service_entry->session_id,
		prot::util::MSG_TYPE_STR(hdr->mt),
		buf.data_len);

	ErrCode result = m_sess_mgr->SendData(service_entry->session_id, buf);
	if (result != ERR_CODE_OK) {
		LOG_ERR("Send message to route service failed");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ProxyService::NotifyClientOffline(uint32_t app_id, uint32_t client_id,
	net::SessionId sid)
{
	prot::util::ClientOfflineReqParam req_param;
	req_param.app_id = app_id;
	req_param.client_id = client_id;
	req_param.session_id = sid;
	req_param.instance_id = m_config.service_config.instance_id;

	com::SigHdrParam hdr_param;
	hdr_param.app_id = app_id;

	com::Buffer buf = prot::util::BuildClientOfflineReq(req_param, hdr_param);

	SendMsgToRouteService(buf); // Notify terminal service

	LOG_INF("Send client offline msg to route service, app:{}, client:{}",
		app_id, client_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ProxyService::OnClientSessionClosed(net::SessionClosedMsgSP data)
{
	LOG_INF("Client session closed, lsid:{}, rsid:{}", data->lsid, data->rsid);

	uint64_t key = m_route_entry_mgr->GetClientRouteEntryKey(data->lsid);
	if (key != 0) {
		NotifyClientOffline(util::FirstInt(key), util::SecondInt(key), data->lsid);
		m_route_entry_mgr->OnSessionClosed(data->lsid);
	}
	else {
		LOG_WRN("Cannot find client route entry, lsid:{}", data->lsid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ProxyService::OnServiceSessionClosed(net::SessionClosedMsgSP data)
{
	LOG_INF("Service session closed, lsid:{}, rsid:{}", data->lsid, data->rsid);

	m_route_service_mgr->RemoveRouteService(data->lsid);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ProxyService::OnSessionClosed(const CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(net::SessionClosedMsg);

	if (data->role == net::SessionRole::CLIENT) {
		OnServiceSessionClosed(data);
	}
	else if (data->role == net::SessionRole::SERVER) {
		OnClientSessionClosed(data);
	}
	else {
		LOG_ERR("Invalid session role:{}", data->role);
	}
}

//------------------------------------------------------------------------------
// Route service session
//------------------------------------------------------------------------------
void ProxyService::OnSessionCreateResult(const CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(net::SessionCreateResultMsg);

	LOG_INF("Session create notify, lsid:{}, rsid:{}, result:{}, addr:{}",
		data->lsid, data->rsid, data->result, data->addr.ToStr());

	for (auto service : m_config.route_services) {
		if (service.service_addr == data->addr) {
			m_route_service_mgr->AddRouteService(service, data->lsid);
		}
	}
}

//------------------------------------------------------------------------------
// Receive message from session, send to service exchange 
//------------------------------------------------------------------------------
void ProxyService::OnSessionData(const CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(net::SessionDataMsg);

	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)data->buf.data.get();

	LOG_INF("Recevied session data, sid:{}, msg:{}, len:{}", data->lsid,
		prot::util::MSG_TYPE_STR(sig_hdr->mt), data->buf.data_len);

	prot::util::DumpSignalHeader(g_proxy_logger, sig_hdr);

	if (sig_hdr->mt == prot::MSG_SERVICE_PONG) {
		m_pinger->OnRecvPongMsg(data->buf);
	}
	else if (sig_hdr->mt == prot::MSG_CLIENT_OFFLINE_RSP) {
		LOG_INF("Received client offline response");
	}
	else {
		if (data->role == net::SessionRole::CLIENT) {
			m_service_msg_processor.Process(data);
		}
		else if (data->role == net::SessionRole::SERVER) {
			m_client_msg_processor.Process(data);
		}
		else {
			LOG_ERR("Unexpected session role:{}", data->role);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ProxyService::OnSessionIncomming(const CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(net::SessionIncommingMsg);

	LOG_INF("Incomming session, lsid:{}, rsid:{}, addr:{}", data->lsid,
		data->rsid, data->addr.ToStr());
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ProxyService::OnThreadMsg(const CommonMsg& msg)
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
	default:
		LOG_ERR("Unknown message type:{}", msg.msg_type);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ProxyService::SendPing(const com::ServiceParam& param, const Buffer& buf)
{
	std::optional<RouteServiceEntry> entry =
		m_route_service_mgr->GetRouteServiceEntry(param.instance_id);
	if (!entry.has_value()) {
		LOG_ERR("cannot find route session to send ping message, type:{}, name:{},"
			" instance:{}", param.service_type, param.service_name, param.instance_id);
		return;
	}

	if (ERR_CODE_OK != m_sess_mgr->SendData(entry->session_id, buf)) {
		LOG_ERR("send ping message failed");
	}

	LOG_DBG("Send ping message, type:{}, name:{}, instance:{}",
		param.service_type, param.service_name, param.instance_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ProxyService::OnPingResult(const com::ServiceParam& param, bool result)
{
	LOG_DBG("Received ping result, result:{}, type:{}, name:{}, instance:{}",
		result, param.service_type, param.service_name, param.instance_id);

	m_reporter->UpdateReportEntry(param.instance_id, (uint32_t)(result ? 0 : 1));
}

}