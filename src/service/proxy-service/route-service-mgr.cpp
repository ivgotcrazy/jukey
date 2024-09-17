#include <random>

#include "route-service-mgr.h"
#include "log.h"


using namespace jukey::net;

namespace
{
	uint32_t GenRand(uint32_t n)
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(0, n);
		return dis(gen);
	}
}

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteServiceMgr::AddRouteService(const RouteServiceConfig& service,
	net::SessionId sid)
{
	RouteServiceEntry entry;
	entry.session_id = sid;
	entry.service_type = service.service_type;
	entry.service_name = service.service_name;
	entry.service_addr = service.service_addr;
	entry.instance_id = service.instance_id;

	m_route_services.push_back(entry);

	LOG_INF("Add route service entry, sid:{}, type:{}, name:{}, instance:{}, "
		"addr:{}",
		sid,
		service.service_type,
		service.service_name,
		service.instance_id,
		service.service_addr.ToStr());
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteServiceMgr::RemoveRouteService(SessionId sid)
{
	for (auto i = m_route_services.begin(); i != m_route_services.end(); i++) {
		if (i->session_id == sid) {
			LOG_INF("Remove route service entry, addr:{}, sid:{}",
				i->service_addr.ToStr(), sid);
			m_route_services.erase(i);
			break;
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::optional<RouteServiceEntry> RouteServiceMgr::GetAvaliableRouteServiceEntry()
{
	if (m_route_services.empty()) {
		return std::nullopt;
	}
	else if (m_route_services.size() == 1) {
		return m_route_services[0];
	}
	else {
		return m_route_services[GenRand((uint32_t)(m_route_services.size() - 1))];
	}

	return std::nullopt;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::optional<RouteServiceEntry>
	RouteServiceMgr::GetRouteServiceEntry(net::SessionId sid)
{
	for (auto i = m_route_services.begin(); i != m_route_services.end(); i++) {
		if (i->session_id == sid) {
			return *i;
		}
	}
	return std::nullopt;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::optional<RouteServiceEntry>
	RouteServiceMgr::GetRouteServiceEntry(const std::string& instance)
{
	for (auto i = m_route_services.begin(); i != m_route_services.end(); i++) {
		if (i->instance_id == instance) {
			return *i;
		}
	}
	return std::nullopt;
}

}