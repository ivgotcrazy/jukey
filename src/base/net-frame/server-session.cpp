#include "server-session.h"
#include "net-message.h"
#include "session-mgr.h"
#include "common/util-time.h"
#include "log.h"


using namespace jukey::com;

namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ServerSession::ServerSession(SessionMgr* mgr, SessionThreadSP st,
	const SessionParam& sparam, const SessionMgrParam& mparam)
	: SessionBase(mgr, st, sparam)
	, m_mgr_param(mparam)
{
	LOG_DBG("[session:{}] New server session", m_sess_param.local_sid);

	m_sess_state = SESSION_STATE_INITIALIZED;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ServerSession::~ServerSession()
{
	LOG_DBG("[session:{}] Destruct server session, rsid:{}",
		m_sess_param.local_sid, m_sess_param.remote_sid);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerSession::DoInit()
{
	m_wait_handshake_ts = util::Now();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerSession::UpdateHandshakingState()
{
	assert(m_wait_handshake_ts != 0);

	// Since the handshake request has not yet been received, the timeout interval
	// of peer is not yet known, then use 3s as timeout interval
	if (m_wait_handshake_ts + 3 * 3000000 <= util::Now()) {
		LOG_ERR("[session:{}] Wait handshake timetout, close session", 
			m_sess_param.local_sid);
		m_sess_mgr->InnerCloseSession(m_sess_param.local_sid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerSession::UpdateTransportingState()
{
	uint64_t now = util::Now();

	// No data sent for too long, then send keep alive message 
	if (m_last_send_data_ts + m_sess_param.remote_kai * 1000000 <= now) {
		SendKeepAlive();
		m_last_send_data_ts = now;
	}

	// No data recv for too long, then reconnect
	if (m_last_recv_data_ts + 3 * m_sess_param.local_kai * 1000000 <= now) {
		m_wait_reconnect_ts = now;
		m_sess_state = SESSION_STATE_RECONNECTING;
		LOG_WRN("[session:{}] Enter reconnecting state", m_sess_param.local_sid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerSession::UpdateReconnectingState()
{
	uint64_t now = util::Now();

	// Wait reconnect request for too long, then close
	if (m_wait_reconnect_ts + 3 * m_sess_param.local_kai * 1000000 <= now) {
		LOG_ERR("[session:{}] Wait reconnect timeout, close session", 
			m_sess_param.local_sid);
		m_sess_mgr->InnerCloseSession(m_sess_param.local_sid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerSession::DoUpdate()
{
	switch (m_sess_state) {
	case SESSION_STATE_HANDSHAKING:
		UpdateHandshakingState();
		break;
	case SESSION_STATE_TRANSPORTING:
		UpdateTransportingState();
		break;
	case SESSION_STATE_RECONNECTING:
		UpdateReconnectingState();
		break;
	default:
		; // Do nothing
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ServerSession::CheckHandshakeData(const HandshakeData& data)
{
	if (data.service_type != (uint16_t)m_sess_param.service_type) {
		LOG_ERR("Invalid service type, server:{}, client:{}",
			m_sess_param.service_type, data.service_type);
		return false;
	}

	if (data.session_type != (uint16_t)SessionType::RELIABLE
		&& data.session_type != (uint16_t)SessionType::UNRELIABLE) {
		LOG_ERR("Invalid session type:{}", data.session_type);
		return false;
	}

	if (data.session_type == (uint16_t)SessionType::RELIABLE
		&& !m_mgr_param.reliable) {
		LOG_ERR("Server not support reliable session!");
		return false;
	}

	if (data.session_type == (uint16_t)SessionType::UNRELIABLE
		&& !m_mgr_param.unreliable) {
		LOG_ERR("Server not support unreliable session!");
		return false;
	}

	m_sess_param.session_type = (SessionType)data.session_type;
	m_sess_param.remote_kai = data.ka_interval;
	m_sess_param.peer_seen_ip = data.peer_ip;

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerSession::SendHandshakeRsp()
{
	HandshakeData data;
	data.version      = 1;
	data.ka_interval  = m_sess_param.local_kai;
	data.peer_ip      = 0; // TODO:
	data.service_type = (uint16_t)m_sess_param.service_type;
	data.session_type = (uint16_t)m_sess_param.session_type;
	data.fec_type     = (uint16_t)m_sess_param.fec_type;

	LOG_INF("[session:{}] Send handshake response, rsid:{}, ver:{}, fec:{}, "
		"session:{}, kai:{}, service:{}, peer:{}",
		m_sess_param.local_sid,
		m_sess_param.remote_sid,
		(uint8_t)data.version,
		(uint8_t)data.fec_type,
		(uint8_t)data.session_type,
		(uint8_t)data.ka_interval,
		data.service_type,
		data.peer_ip);

	SendHandshake(data);
}

//------------------------------------------------------------------------------
// Handshake request
//------------------------------------------------------------------------------
void ServerSession::OnHandshake(const SessionPktSP& pkt)
{
	HandshakeData data = SessionProtocol::ParseHandshakeData(pkt->buf);

	LOG_INF("[session:{}] Received handshake request, rsid:{}, ver:{}, fec:{}, "
		"session:{}, kai:{}, service:{}, peer:{}",
		m_sess_param.local_sid,
		m_sess_param.remote_sid,
		(uint8_t)data.version,
		(uint8_t)data.fec_type,
		(uint8_t)data.session_type,
		(uint8_t)data.ka_interval,
		data.service_type,
		data.peer_ip);

	if (!CheckHandshakeData(data)) {
		LOG_ERR("Check handshake data failed!");
		m_sess_mgr->InnerCloseSession(m_sess_param.local_sid);
		return;
	}

	// TODO: negotiate
	m_sess_param.remote_sid = pkt->head.src;
	m_sess_param.remote_kai = data.ka_interval;
	m_sess_param.peer_seen_ip = data.peer_ip;
	m_sess_param.session_type = (SessionType)data.session_type;
	m_sess_param.fec_type = (FecType)data.fec_type;

	SendHandshakeRsp();

	if (m_sess_state == SESSION_STATE_HANDSHAKING) {
		OnSessionNegotiateCompelete();

		// Notify incomming client
		com::CommonMsg msg;
		msg.msg_type = NET_MSG_SESSION_INCOMING;
		msg.msg_data.reset(new SessionIncommingMsg(
			m_sess_param.local_sid,
			m_sess_param.remote_sid,
			com::Address(m_sess_param.remote_addr.ep,
				m_sess_param.remote_addr.type)
		));
		m_sess_param.thread->PostMsg(msg);

		m_sess_state = SESSION_STATE_TRANSPORTING;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerSession::OnReconnectReq(const SessionPktSP& pkt)
{
	LOG_INF("[session:{}] Received reconnect request, rsid:{}",
		m_sess_param.local_sid, m_sess_param.remote_sid);

	m_sess_state = SESSION_STATE_TRANSPORTING;

	SendReconnectRsp();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerSession::OnSessionClose(const SessionPktSP& pkt)
{
	LOG_INF("[session:{}] Received session close, rsid:{}",
		m_sess_param.local_sid, m_sess_param.remote_sid);

	m_sess_mgr->InnerCloseSession(m_sess_param.local_sid);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerSession::OnKeepAlive(const SessionPktSP& pkt)
{
	LOG_INF("[session:{}] Received keep alive response, rsid:{}",
		m_sess_param.local_sid, m_sess_param.remote_sid);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerSession::OnSessionControlMsg(const SessionPktSP& pkt)
{
	switch (pkt->head.pt) {
	case SESSION_PKT_HANDSHAKE:
		OnHandshake(pkt);
		break;
	case SESSION_PKT_KEEP_ALIVE:
		OnKeepAlive(pkt);
		break;
	case SESSION_PKT_RECONNECT_REQ:
		OnReconnectReq(pkt);
		break;
	case SESSION_PKT_CLOSE:
		OnSessionClose(pkt);
		break;
	default:
		LOG_ERR("Unexpected packet:{}, len:{}", (uint8_t)pkt->head.pt, pkt->buf.data_len);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerSession::DoClose()
{
	// TODO: should start close timer and force to close connection
}

}