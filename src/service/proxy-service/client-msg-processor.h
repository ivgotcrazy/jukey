#pragma once

#include "route-entry-mgr.h"
#include "route-service-mgr.h"

namespace jukey::srv
{

//==============================================================================
// 
//==============================================================================
class ClientMsgProcessor
{
public:
	void Init(const ProxyServiceConfig& config,
		net::ISessionMgr* session_mgr,
		RouteEntryMgrSP entry_mgr,
		RouteServiceMgrSP service_mgr);

	void Process(const net::SessionDataMsgSP& msg);

private:
	void SendMsgToService(const net::SessionDataMsgSP& msg);
	void OnClientRegReq(const net::SessionDataMsgSP& msg);
	void OnUserLoginReq(const net::SessionDataMsgSP& msg);
	void OnJoinGroupReq(const net::SessionDataMsgSP& msg);

private:
	// O means invalid value
	struct ClientInfo
	{
		uint32_t app_id = 0;
		uint32_t clt_id = 0;
		uint32_t usr_id = 0;
		uint32_t grp_id = 0;

		bool normal_clt = false;
		bool normal_usr = false;
		bool normal_grp = false;
	};
	typedef std::shared_ptr<ClientInfo> ClientInfoSP;

private:
	ProxyServiceConfig m_config;
	net::ISessionMgr* m_sess_mgr = nullptr;
	RouteEntryMgrSP m_route_entry_mgr;
	RouteServiceMgrSP m_route_service_mgr;
};

}