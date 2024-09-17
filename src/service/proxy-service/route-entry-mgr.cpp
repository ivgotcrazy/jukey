#include "route-entry-mgr.h"
#include "log.h"
#include "protocol.h"
#include "common/util-common.h"
#include "proxy-common.h"
#include "sig-msg-builder.h"


using namespace jukey::prot;
using namespace jukey::net;
using namespace jukey::util;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteEntryMgr::OnClientRegReq(const SessionDataMsgSP& msg)
{
	LOG_INF("Process route entry on receiving client register request");

	SigMsgHdr* hdr = (SigMsgHdr*)msg->buf.data.get();
	APP_CHECK_AND_RETURN(hdr);
	CLT_CHECK_AND_RETURN(hdr);

	// Make route key
	uint64_t route_key = util::CombineTwoInt(hdr->app, hdr->clt);

	// Client route should not exist
	auto route_iter = m_client_routes.find(route_key);
	if (route_iter != m_client_routes.end()) {
		LOG_WRN("Client route already exists, remove it!");
		m_client_routes.erase(route_iter);
	}

	CltRouteEntry entry;
	entry.sid = msg->lsid;
	entry.normal = false;

	m_client_routes.insert(std::make_pair(route_key, entry));

	LOG_INF("Add new client route entry, app:{}, client:{}, sid:{}",
		hdr->app, hdr->clt, msg->lsid);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteEntryMgr::OnClientRegRsp(const SessionDataMsgSP& msg)
{
	LOG_INF("Process route entry on receiving client register response");

	SigMsgHdr* hdr = (SigMsgHdr*)msg->buf.data.get();
	APP_CHECK_AND_RETURN(hdr);
	CLT_CHECK_AND_RETURN(hdr);

	// Client route entry should exist
	auto riter = m_client_routes.find(CombineTwoInt(hdr->app, hdr->clt));
	if (riter == m_client_routes.end()) {
		LOG_ERR("Cannot find client route, app:{}, client:{}", hdr->app, hdr->clt);
		return;
	}

	// Register success
	if (hdr->c == 0) {
		riter->second.normal = true;
	}
	else { // Register failed
		m_client_routes.erase(riter);

		LOG_INF("Erase client routes item, app:{}, client:{}, lsid:{}",
			hdr->app, hdr->clt, msg->lsid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteEntryMgr::OnUserLoginReq(const SessionDataMsgSP& msg)
{
	LOG_INF("Process route entry on receiving user login request");

	SigMsgHdr* hdr = (SigMsgHdr*)msg->buf.data.get();
	APP_CHECK_AND_RETURN(hdr);
	CLT_CHECK_AND_RETURN(hdr);
	USR_CHECK_AND_RETURN(hdr);

	// Make route key
	uint64_t route_key = util::CombineTwoInt(hdr->app, hdr->usr);

	// User route should not exist
	auto riter = m_user_routes.find(route_key);
	if (riter != m_user_routes.end()) {
		LOG_WRN("User route already exists, remove it!");
		m_user_routes.erase(riter);
	}

	// Add user route entry
	UsrRouteEntry entry;
	entry.sid = msg->lsid;
	entry.client_id = hdr->clt;
	entry.normal = false;
	m_user_routes.insert(std::make_pair(route_key, entry));

	LOG_INF("Add user route entry, app:{}, user:{}, lsid:{}", hdr->app, hdr->usr,
		msg->lsid);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteEntryMgr::OnUserLoginRsp(const SessionDataMsgSP& msg)
{
	LOG_INF("Process route entry on receiving user login response");

	SigMsgHdr* hdr = (SigMsgHdr*)msg->buf.data.get();
	APP_CHECK_AND_RETURN(hdr);
	CLT_CHECK_AND_RETURN(hdr);
	USR_CHECK_AND_RETURN(hdr);

	// User route entry should exist
	auto riter = m_user_routes.find(CombineTwoInt(hdr->app, hdr->usr));
	if (riter == m_user_routes.end()) {
		LOG_ERR("Cannot find user route, app:{}, user:{}", hdr->app, hdr->usr);
		return;
	}

	// Login success
	if (hdr->c == 0) {
		riter->second.normal = true;
	}
	else { // Login failed
		m_user_routes.erase(riter);

		LOG_INF("Erase user routes item, app:{}, user:{}, lsid:{}",
			hdr->app, hdr->usr, msg->lsid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteEntryMgr::OnJoinGroupReq(const SessionDataMsgSP& msg)
{
	LOG_INF("Process route entry on receiving join group request");

	SigMsgHdr* hdr = (SigMsgHdr*)msg->buf.data.get();
	APP_CHECK_AND_RETURN(hdr);
	CLT_CHECK_AND_RETURN(hdr);
	USR_CHECK_AND_RETURN(hdr);
	GRP_CHECK_AND_RETURN(hdr);

	// Make route key
	uint64_t route_key = util::CombineTwoInt(hdr->app, hdr->grp);

	auto giter = m_group_routes.find(route_key);
	if (giter == m_group_routes.end()) { // new group route entry
		GrpRouteEntry route_entry;
		route_entry.insert(std::make_pair(msg->lsid, false));
		m_group_routes.insert(std::make_pair(route_key, route_entry));
		LOG_INF("New group route entry, group:{}, sid:{}", hdr->grp, msg->lsid);
	}
	else {
		auto riter = giter->second.find(msg->lsid);
		if (riter != giter->second.end()) {
			LOG_WRN("Remove group route entry, lsid:{}", msg->lsid);
			giter->second.erase(msg->lsid);
		}
		giter->second.insert(std::make_pair(msg->lsid, false));
		LOG_INF("Add item to group route entry, group:{}, sid:{}", hdr->grp, msg->lsid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteEntryMgr::OnJoinGroupRsp(const SessionDataMsgSP& msg)
{
	LOG_INF("Process route entry on receiving join group response");

	SigMsgHdr* hdr = (SigMsgHdr*)msg->buf.data.get();
	APP_CHECK_AND_RETURN(hdr);
	CLT_CHECK_AND_RETURN(hdr);
	USR_CHECK_AND_RETURN(hdr);

	// User route entry should exist
	auto uiter = m_user_routes.find(CombineTwoInt(hdr->app, hdr->usr));
	if (uiter == m_user_routes.end()) {
		LOG_ERR("Cannot find user route, app:{}, user:{}", hdr->app, hdr->usr);
		return;
	}

	// Join group failed
	if (hdr->g == 1) {
		LOG_WRN("Join group:{} failed!", hdr->grp);
		return; // TODO: check session
	}

	// Make group route key
	uint64_t group_key = util::CombineTwoInt(hdr->app, hdr->grp);

	// Group route not exists, then add it
	auto giter = m_group_routes.find(group_key);
	if (giter == m_group_routes.end()) {
		LOG_INF("Cannot find group route, user:{}, group:{}, app:{}",
			hdr->usr, hdr->grp, hdr->app);
		return;
	}

	// Group route already exists, then add session entry
	auto riter = giter->second.find(uiter->second.sid);
	if (riter == giter->second.end()) {
		LOG_WRN("Cannot find group route entry, group:{}, session:{}!", hdr->grp,
			uiter->second.sid);
	}
	else {
		riter->second = true;
		LOG_INF("Add group route entry, user:{}, group:{}, app:{}, session:{}",
			hdr->usr, hdr->grp, hdr->app, riter->first);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
CltRouteEntryOpt RouteEntryMgr::GetClientRouteEntry(uint32_t app_id,
	uint32_t client_id)
{
	auto iter = m_client_routes.find(CombineTwoInt(app_id, client_id));
	if (iter == m_client_routes.end()) {
		LOG_ERR("Cannot find client route entry, app:{}, client:{}", app_id, client_id);
		return std::nullopt;
	}

	return iter->second;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
UsrRouteEntryOpt RouteEntryMgr::GetUserRouteEntry(uint32_t app_id,
	uint32_t user_id)
{
	auto iter = m_user_routes.find(CombineTwoInt(app_id, user_id));
	if (iter == m_user_routes.end()) {
		LOG_ERR("Cannot find user route entry, app:{}, user:{}", app_id, user_id);
		return std::nullopt;
	}

	return iter->second;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
GrpRouteEntryOpt RouteEntryMgr::GetGroupRouteEntry(uint32_t app_id,
	uint32_t group_id)
{
	auto iter = m_group_routes.find(CombineTwoInt(app_id, group_id));
	if (iter == m_group_routes.end()) {
		LOG_ERR("Cannot find group route entry, app:{}, group:{}", app_id, group_id);
		return std::nullopt;
	}

	if (iter->second.empty()) {
		LOG_WRN("Empty session set, app:{}, group:{}", app_id, group_id);
		return std::nullopt;
	}

	return iter->second;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint64_t RouteEntryMgr::GetClientRouteEntryKey(net::SessionId sid)
{
	for (auto item : m_client_routes) {
		if (item.second.sid == sid) {
			return item.first;
		}
	}
	return 0;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RouteEntryMgr::OnSessionClosed(net::SessionId sid)
{
	LOG_INF("Session closed, sid:{}", sid);

	uint32_t app_id = 0;
	uint32_t client_id = 0;

	// Remove client route entry
	for (auto iter = m_client_routes.begin(); iter != m_client_routes.end();) {
		if (iter->second.sid == sid) {
			app_id = util::FirstInt(iter->first);
			client_id = util::SecondInt(iter->first);
			LOG_INF("Remove client route entry, client:{}", client_id);
			iter = m_client_routes.erase(iter);
		}
		else {
			++iter;
		}
	}

	if (client_id == 0) {
		LOG_ERR("Cannot find session in client route entry");
		return;
	}

	// Remove user route entry
	for (auto iter = m_user_routes.begin(); iter != m_user_routes.end();) {
		if (iter->second.client_id == client_id) {
			LOG_INF("Remove user route entry, user:{}", util::SecondInt(iter->first));
			iter = m_user_routes.erase(iter);
		}
		else {
			++iter;
		}
	}
}

}