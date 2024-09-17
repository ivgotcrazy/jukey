#pragma once

#include <optional>

#include "common-struct.h"
#include "if-session-mgr.h"
#include "proxy-common.h"
#include "net-message.h"
#include "config-parser.h"

namespace jukey::srv
{

struct RouteServiceEntry
{
	uint32_t service_type = 0;
	std::string service_name;
	std::string instance_id;
	com::Address service_addr;
	net::SessionId session_id = 0;
};

//==============================================================================
// 
//==============================================================================
class RouteServiceMgr
{
public:
	void AddRouteService(const RouteServiceConfig& service, net::SessionId sid);
	void RemoveRouteService(net::SessionId sid);

	std::optional<RouteServiceEntry> GetAvaliableRouteServiceEntry();
	std::optional<RouteServiceEntry> GetRouteServiceEntry(net::SessionId sid);
	std::optional<RouteServiceEntry> GetRouteServiceEntry(const std::string& instance);

private:
	// All messages transmit to route services
	std::vector<RouteServiceEntry> m_route_services;
};
typedef std::shared_ptr<RouteServiceMgr> RouteServiceMgrSP;

}