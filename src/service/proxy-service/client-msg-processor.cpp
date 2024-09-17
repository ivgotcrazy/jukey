#include "client-msg-processor.h"
#include "log.h"
#include "protocol.h"
#include "util-protocol.h"
#include "protoc/terminal.pb.h"


using namespace jukey::net;
using namespace jukey::prot;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ClientMsgProcessor::Init(const ProxyServiceConfig& config, 
	ISessionMgr* session_mgr, RouteEntryMgrSP entry_mgr, 
	RouteServiceMgrSP service_mgr)
{
	m_config = config;
	m_sess_mgr = session_mgr;
	m_route_entry_mgr = entry_mgr;
	m_route_service_mgr = service_mgr;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ClientMsgProcessor::OnClientRegReq(const SessionDataMsgSP& msg)
{
	m_route_entry_mgr->OnClientRegReq(msg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ClientMsgProcessor::OnUserLoginReq(const SessionDataMsgSP& msg)
{
	m_route_entry_mgr->OnUserLoginReq(msg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ClientMsgProcessor::OnJoinGroupReq(const SessionDataMsgSP& msg)
{
	m_route_entry_mgr->OnJoinGroupReq(msg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ClientMsgProcessor::SendMsgToService(const SessionDataMsgSP& msg)
{
	std::optional<RouteServiceEntry> service_entry =
		m_route_service_mgr->GetAvaliableRouteServiceEntry();
	if (!service_entry.has_value()) {
		LOG_ERR("Cannot get avaliable route service!");
		return;
	}

	SigMsgHdr* sig_hdr = (SigMsgHdr*)DP(msg->buf);

	if (sig_hdr->mt == MSG_CLIENT_REGISTER_REQ) {
		prot::RegisterReqExtendData extend_data;
		extend_data.set_instance_id(m_config.service_config.instance_id);
		extend_data.set_session_id(msg->lsid);

		// Alloc a new buffer to store message data and extend data
		com::Buffer new_buf(msg->buf.data_len + extend_data.ByteSizeLong());
		new_buf.data_len = new_buf.total_len;

		// Copy message data
		memcpy(new_buf.data.get(), DP(msg->buf), msg->buf.data_len);

		// Write extend data
		extend_data.SerializeToArray(new_buf.data.get() + msg->buf.data_len,
			extend_data.ByteSizeLong());

		// Mark has extend data
		SigMsgHdr* new_sig_hdr = (SigMsgHdr*)DP(new_buf);
		new_sig_hdr->e = 1;

		m_sess_mgr->SendData(service_entry.value().session_id, new_buf);

		LOG_INF("Send message to route service, addr:{}, sid:{}, msg:{}, len:{}",
			service_entry.value().service_addr.ToStr(),
			service_entry.value().session_id,
			prot::util::MSG_TYPE_STR(sig_hdr->mt),
			new_buf.data_len);
	}
	else {
		m_sess_mgr->SendData(service_entry.value().session_id, msg->buf);

		LOG_INF("Send message to route service, addr:{}, sid:{}, msg:{}, len:{}",
			service_entry.value().service_addr.ToStr(),
			service_entry.value().session_id,
			prot::util::MSG_TYPE_STR(sig_hdr->mt),
			msg->buf.data_len);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ClientMsgProcessor::Process(const SessionDataMsgSP& msg)
{
	// Pasre data
	SigMsgHdr* sig_hdr = (SigMsgHdr*)(msg->buf.data.get());

	// Process route entry first
	switch (sig_hdr->mt) {
	case MSG_CLIENT_REGISTER_REQ:
		OnClientRegReq(msg);
		break;
	case MSG_USER_LOGIN_REQ:
		OnUserLoginReq(msg);
		break;
	case MSG_JOIN_GROUP_REQ:
		OnJoinGroupReq(msg);
		break;
	}

	SendMsgToService(msg);
}

}