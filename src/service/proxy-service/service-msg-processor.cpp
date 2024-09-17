#include "service-msg-processor.h"
#include "protocol.h"
#include "log.h"
#include "proxy-common.h"
#include "util-protocol.h"


using namespace jukey::net;
using namespace jukey::prot;
using namespace jukey::com;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServiceMsgProcessor::Init(ISessionMgr* session_mgr, RouteEntryMgrSP entry_mgr)
{
	m_sess_mgr = session_mgr;
	m_route_entry_mgr = entry_mgr;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServiceMsgProcessor::OnClientRegRsp(const SessionDataMsgSP& msg)
{
	m_route_entry_mgr->OnClientRegRsp(msg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServiceMsgProcessor::OnUserLoginRsp(const SessionDataMsgSP& msg)
{
	m_route_entry_mgr->OnUserLoginRsp(msg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServiceMsgProcessor::OnJoinGroupRsp(const SessionDataMsgSP& msg)
{
	m_route_entry_mgr->OnJoinGroupRsp(msg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServiceMsgProcessor::SendUnicastMsgToClient(const com::Buffer& msg)
{
	SigMsgHdr* hdr = (SigMsgHdr*)msg.data.get();

	CltRouteEntryOpt entry = m_route_entry_mgr->GetClientRouteEntry(hdr->app,
		hdr->clt);
	if (!entry.has_value()) {
		LOG_ERR("Cannot find client route entry, app:{}, client:{}",
			hdr->app, hdr->clt);
		return;
	}

	if (!entry->normal) {
		LOG_WRN("Client route entry is unnormal, app:{}, client:{}",
			hdr->app, hdr->clt);
	}

	ErrCode result = m_sess_mgr->SendData(entry->sid, msg);
	if (ERR_CODE_OK != result) {
		LOG_ERR("Send data failed, session:{}", entry->sid);
	}

	LOG_INF("send unicast message to client, msg:{}, client:{}, len:{}",
		prot::util::MSG_TYPE_STR(hdr->mt), hdr->clt, msg.data_len);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServiceMsgProcessor::SendUnicastMsgToUser(const com::Buffer& msg)
{
	SigMsgHdr* hdr = (SigMsgHdr*)msg.data.get();

	UsrRouteEntryOpt entry = m_route_entry_mgr->GetUserRouteEntry(hdr->app,
		hdr->usr);
	if (!entry.has_value()) {
		LOG_ERR("Cannot get user route entry, app:{}, user:{}", hdr->app, hdr->usr);
		return;
	}

	if (!entry->normal) {
		LOG_WRN("User route entry is unnormal, app:{}, user:{}", hdr->app,
			hdr->usr);
	}

	ErrCode result = m_sess_mgr->SendData(entry->sid, msg);
	if (ERR_CODE_OK != result) {
		LOG_ERR("Send data failed, session:{}", entry->sid);
	}

	LOG_INF("send unicast message to user, msg:{}, user:{}, len:{}",
		prot::util::MSG_TYPE_STR(hdr->mt), hdr->usr, msg.data_len);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServiceMsgProcessor::SendMulticastMsgToUsers(const com::Buffer& msg)
{
	SigMsgHdr* hdr = (SigMsgHdr*)msg.data.get();

	LOG_INF("Start to send multicast msg:{}", (uint32_t)hdr->mt);

	APP_CHECK_AND_RETURN(hdr);
	//CLT_CHECK_AND_RETURN(hdr);
	//USR_CHECK_AND_RETURN(hdr);
	GRP_CHECK_AND_RETURN(hdr);

	GrpRouteEntryOpt entry = m_route_entry_mgr->GetGroupRouteEntry(hdr->app, hdr->grp);
	if (!entry.has_value()) {
		LOG_ERR("Cannot get group route entry, app:{}, grp:{}", hdr->app, hdr->grp);
		return;
	}

	uint32_t send_count = 0;

	for (auto& item : entry.value()) {
		if (!item.second) {
			LOG_WRN("Unnormal route entry, sid:{}", item.first);
		}

		if (ERR_CODE_OK != m_sess_mgr->SendData(item.first, msg)) {
			LOG_ERR("Send data failed, session:{}", item.first);
		}

		LOG_INF("send multicast msg:{} to user, sid:{}", (uint32_t)hdr->mt,
			item.first);

		++send_count;
	}

	LOG_INF("send multicast message to users, msg:{}, users:{}, len:{}",
		prot::util::MSG_TYPE_STR(hdr->mt), send_count, msg.data_len);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServiceMsgProcessor::SendMsgToUser(const Buffer& msg)
{
	SigMsgHdr* sig_hdr = (SigMsgHdr*)msg.data.get();

	if (sig_hdr->grp != 0) {
		if (sig_hdr->usr != 0) {
			SendUnicastMsgToUser(msg);
		}
		else {
			SendMulticastMsgToUsers(msg);
		}
	}
	else { // grp == 0
		if (sig_hdr->usr != 0) {
			SendUnicastMsgToUser(msg);
		}
		else {
			if (sig_hdr->clt != 0) {
				SendUnicastMsgToClient(msg);
			}
			else {
				LOG_WRN("Not implemented!"); // TODO: app broadcast
			}
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServiceMsgProcessor::Process(const SessionDataMsgSP& msg)
{
	// Pasre data
	// 
	SigMsgHdr* sig_hdr = (SigMsgHdr*)DP(msg->buf);

	// Send the message first, otherwise the route entry may be deleted
	SendMsgToUser(msg->buf);

	switch (sig_hdr->mt) {
	case MSG_CLIENT_REGISTER_RSP:
		OnClientRegRsp(msg);
		break;
	case MSG_USER_LOGIN_RSP:
		OnUserLoginRsp(msg);
		break;
	case MSG_JOIN_GROUP_RSP:
		OnJoinGroupRsp(msg);
		break;
	}
}

}