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
#include "if-amqp-client.h"
#include "proxy-common.h"
#include "route-entry-mgr.h"
#include "route-service-mgr.h"
#include "if-reporter.h"
#include "if-pinger.h"
#include "config-parser.h"
#include "client-msg-processor.h"
#include "service-msg-processor.h"

namespace jukey::srv
{

//==============================================================================
// 
//==============================================================================
class ProxyService 
	: public IService
	, public base::ProxyUnknown
	, public base::ComObjTracer
	, public util::CommonThread
	, public com::IPingHandler
{
public:
	ProxyService(base::IComFactory* factory, const char* owner);

	// ProxyUnknown
	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IService
	virtual bool Init(net::ISessionMgr* mgr, const std::string& cfg_file) override;
	virtual bool Start() override;
	virtual void Stop() override;
	virtual bool Reload() override;

	// CommonThread
	virtual void OnThreadMsg(const com::CommonMsg& msg) override;

	// IPingHandler
	virtual void SendPing(const com::ServiceParam& param,
		const com::Buffer& buf) override;
	virtual void OnPingResult(const com::ServiceParam& param,
		bool result) override;

private:
	void OnSessionClosed(const com::CommonMsg& msg);
	void OnSessionCreateResult(const com::CommonMsg& msg);
	void OnSessionData(const com::CommonMsg& msg);
	void OnSessionIncomming(const com::CommonMsg& msg);
	void OnClientSessionClosed(net::SessionClosedMsgSP data);
	void OnServiceSessionClosed(net::SessionClosedMsgSP data);
	bool DoListen();
	bool ConnectRouteServices();
	bool DoInitReport();
	bool DoInitPinger();
	void SendMsgToRouteService(const com::Buffer& buf);
	void NotifyClientOffline(uint32_t app_id, uint32_t client_id,
		net::SessionId sid);
	
private:
	base::IComFactory* m_factory = nullptr;
	net::ISessionMgr* m_sess_mgr = nullptr;
	com::IReporter* m_reporter = nullptr;
	com::IPinger* m_pinger = nullptr;
	RouteServiceMgrSP m_route_service_mgr;
	RouteEntryMgrSP m_route_entry_mgr;
	net::ListenId m_listen_id = INVALID_LISTEN_ID;
	ProxyServiceConfig m_config;
	ClientMsgProcessor m_client_msg_processor;
	ServiceMsgProcessor m_service_msg_processor;
};

}