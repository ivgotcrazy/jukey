#pragma once

#include "if-session-mgr.h"
#include "net-message.h"
#include "proxy-common.h"

namespace jukey::srv
{

//==============================================================================
// 
//==============================================================================
class RouteEntryMgr
{
public:
	void OnClientRegReq(const net::SessionDataMsgSP& msg);
	void OnClientRegRsp(const net::SessionDataMsgSP& msg);
	void OnUserLoginReq(const net::SessionDataMsgSP& msg);

	void OnUserLoginRsp(const net::SessionDataMsgSP& msg);
	void OnJoinGroupReq(const net::SessionDataMsgSP& msg);
	void OnJoinGroupRsp(const net::SessionDataMsgSP& msg);

	void OnSessionClosed(net::SessionId sid);

	CltRouteEntryOpt GetClientRouteEntry(uint32_t app_id, uint32_t client_id);
	UsrRouteEntryOpt GetUserRouteEntry(uint32_t app_id, uint32_t user_id);
	GrpRouteEntryOpt GetGroupRouteEntry(uint32_t app_id, uint32_t group_id);

	uint64_t GetClientRouteEntryKey(net::SessionId sid);

private:
	// key:AppId(32bits)+UserId(32bits), to accelerate user route looking up
	std::unordered_map<uint64_t, UsrRouteEntry> m_user_routes;

	// key:AppId(32bits)+ClientId(32bits), to accelerate client route looking up
	std::unordered_map<uint64_t, CltRouteEntry> m_client_routes;

	// key:AppId(32bit)+GroupId(32bits), to accelerate group route looking up
	std::unordered_map<uint64_t, GrpRouteEntry> m_group_routes;
};
typedef std::shared_ptr<RouteEntryMgr> RouteEntryMgrSP;

}