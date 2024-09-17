#pragma once

#include <unordered_map>
#include <set>
#include <vector>

#include "if-service.h"
#include "com-factory.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "thread/common-thread.h"
#include "if-session-mgr.h"
#include "mq-msg-sender.h"
#include "if-amqp-client.h"
#include "protocol.h"
#include "if-reporter.h"
#include "if-pinger.h"
#include "config-parser.h"

namespace jukey::srv
{

//==============================================================================
// 
//==============================================================================
class RouteService 
	: public IService
	, public base::ProxyUnknown
	, public base::ComObjTracer
	, public util::CommonThread
	, public com::IAmqpHandler
	, public com::IPingHandler
{
public:
	RouteService(base::IComFactory* factory, const char* owner);

	// ProxyUnknown
	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IService
	virtual bool Init(net::ISessionMgr* mgr, 
		const std::string& config_file) override;
	virtual bool Start() override;
	virtual void Stop() override;
	virtual bool Reload() override;

	// CommonThread
	virtual void OnThreadMsg(const com::CommonMsg& msg) override;

	// IAmqpHandler
	virtual void OnRecvMqMsg(const std::string& queue, 
		const com::Buffer& mq_buf, 
		const com::Buffer& sig_buf) override;

	// IPingHandler
	virtual void SendPing(const com::ServiceParam& param,
		const com::Buffer& buf) override;
	virtual void OnPingResult(const com::ServiceParam& param,
		bool result) override;

private:
	struct ProxyRouteInfo
	{
		com::Address remote_addr;
	};
	typedef std::shared_ptr<ProxyRouteInfo> ProxyRouteInfoSP;

	// Key:SessionId, value:timestamp
	typedef std::unordered_map<net::SessionId, uint64_t> RouteMap;

	struct RouteEntry
	{
		RouteEntry(net::SessionId s, uint64_t t) : sid(s), last_update(t) {}

		net::SessionId sid = 0;
		uint64_t last_update = 0;
	};

private:
	void OnSessionClosed(const com::CommonMsg& msg);
	void OnSessionCreateResult(const com::CommonMsg& msg);
	void OnSessionData(const com::CommonMsg& msg);
	void OnSessionIncomming(const com::CommonMsg& msg);
	void OnMqMsg(const com::CommonMsg& msg);
	void OnSendPing(const com::CommonMsg& msg);
	void OnPingResult(const com::CommonMsg& msg);

	void OnServicePingMsg(net::SessionId sid, const com::Buffer& buf);
	bool ContructRouteMap();
	bool DoInitListen();
	bool DoInitMq();
	bool DoInitSerivceMq();
	bool DoInitPongMq();
	bool DoInitReport();
	bool DoInitPinger();
	
	void SendMsgToService(const com::Buffer& msg);
	void SendMsgToProxy(const com::Buffer& sig_buf);
	void SendMulticastMsgToProxy(const com::Buffer& sig_buf);
	void SendUserUnicastMsgToProxy(const com::Buffer& sig_buf);
	void SendClientUnicastMsgToProxy(const com::Buffer& sig_buf);
	void SendBroadcastMsgToProxy(const com::Buffer& sig_buf);

	void LearnRouteEntry(net::SessionId sid, prot::SigMsgHdr* sig_hdr);
	void LearnAppRouteEntry(net::SessionId sid, prot::SigMsgHdr* sig_hdr);
	void LearnClientRouteEntry(net::SessionId sid, prot::SigMsgHdr* sig_hdr);
	void LearnUserRouteEntry(net::SessionId sid, prot::SigMsgHdr* sig_hdr);
	void LearnGroupRouteEntry(net::SessionId sid, prot::SigMsgHdr* sig_hdr);

private:
	base::IComFactory* m_factory = nullptr;
	net::ISessionMgr* m_sess_mgr = nullptr;
	RouteServiceConfig m_config;
	net::ListenId m_listen_id = INVALID_LISTEN_ID;
	MqMsgSender m_mq_msg_sender;
	com::IAmqpClient* m_amqp_client = nullptr;
	com::IReporter* m_reporter = nullptr;
	com::IPinger* m_pinger = nullptr;
	std::string m_service_queue;
	std::string m_ping_queue;

	// Connections from proxy services
	std::unordered_map<net::SessionId, ProxyRouteInfoSP> m_proxy_services;

	// key:AppId(32bit)+ClientId(32bit), for client unicast
	std::unordered_map<uint64_t, RouteEntry> m_client_route_tbl;

	// key:AppId(32bit)+UserId(32bit), for user unicast
	std::unordered_map<uint64_t, RouteEntry> m_user_route_tbl;

	// Key:AppId(32bit), for app broadcast
	std::unordered_map<uint32_t, RouteMap> m_app_route_tbl;

	// Key:AppId(32bit)+GroupId(32bit), for group multicast
	std::unordered_map<uint64_t, RouteMap> m_group_route_tbl;

	// message:exchange
	std::unordered_map<uint32_t, std::string> m_service_routes;
};

}