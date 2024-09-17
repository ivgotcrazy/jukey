#include "client-session.h"
#include "net-message.h"
#include "session-mgr.h"
#include "common/util-time.h"
#include "log.h"

using namespace jukey::util;

namespace jukey::net
{
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ClientSession::ClientSession(SessionMgr* mgr, SessionThreadSP st, 
	const SessionParam& sparam, const CreateParam& cparam)
	: SessionBase(mgr, st, sparam)
  , m_create_param(cparam)
{
  LOG_DBG("[session:{}] New client session", m_sess_param.local_sid);

  m_sess_state = SESSION_STATE_INITIALIZED;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ClientSession::~ClientSession()
{
	LOG_DBG("[session:{}] Destruct client session, remote session:{}", 
		m_sess_param.local_sid, m_sess_param.remote_sid);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ClientSession::SendHandshakeReq()
{
  HandshakeData data;
  data.version      = 1;
  data.ka_interval  = m_create_param.ka_interval;
  data.peer_ip      = 0; // TODO:
  data.service_type = (uint16_t)m_create_param.service_type;
  data.session_type = (uint16_t)m_create_param.session_type;
  data.fec_type     = (uint16_t)m_create_param.fec_type;

  LOG_INF("[session:{}] Send handshake request, rsid:{}, ver:{}, fec:{}, "
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

  m_send_handshake_ts = util::Now();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ClientSession::DoInit()
{
  for (auto i = 0; i < 3; i++) {
    SendHandshakeReq();
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ClientSession::NotifySessionCreateResult(bool result)
{
	com::CommonMsg msg;
	msg.msg_type = NET_MSG_SESSION_CREATE_RESULT;
	msg.msg_data.reset(new SessionCreateResultMsg(
		m_sess_param.local_sid,
		m_sess_param.remote_sid, 
		result, 
		m_create_param.remote_addr));

	m_sess_param.thread->PostMsg(msg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ClientSession::UpdateHandshakingState()
{
  uint64_t now = util::Now();

  // No handshake response for too long, then close session, 3 seconds
  if (m_send_handshake_ts + 3 * 1000000 <= now) {
    LOG_ERR("[session:{}] Wait handshake response failed!",
			m_sess_param.local_sid);

		if (m_handshake_retry_count < 2) {
			for (auto i = 0; i < 3; i++) {
				SendHandshakeReq();
			}
			m_send_handshake_ts = util::Now();
			++m_handshake_retry_count;
		}
		else {
			if (m_sess_state == SESSION_STATE_HANDSHAKING) {
				NotifySessionCreateResult(false);
				m_sess_state = SESSION_STATE_INITIALIZED;
			}
			else {
				m_sess_mgr->InnerCloseSession(m_sess_param.local_sid);
			}
		}
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ClientSession::UpdateTransportingState()
{
  uint64_t now = util::Now();

  // No data sent to server for too long, then send keep alive message 
  if (m_last_send_data_ts + m_sess_param.remote_kai * 1000000 <= now) {
    SendKeepAlive();
    m_last_send_data_ts = now;
  }

  // Receive no data from server for too long, then reconnect
  if (m_last_recv_data_ts + 3 * m_sess_param.local_kai * 1000000 <= now) {
    SendReconnectReq();
    m_last_reconnect_ts = now;
		m_last_send_reconnect_ts = now;
    m_sess_state = SESSION_STATE_RECONNECTING;
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ClientSession::UpdateReconnectingState()
{
	uint64_t now = util::Now();

	if (m_last_send_reconnect_ts + m_sess_param.local_kai * 1000000 <= now) {
		SendReconnectReq();
		m_last_send_reconnect_ts = now;
	}

  // Wait reconnect response for too long, then close
  if (m_last_reconnect_ts + 3 * m_sess_param.local_kai * 1000000 <= now) {
    LOG_WRN("Wait reconnect response failed!");
		m_sess_mgr->InnerCloseSession(m_sess_param.local_sid);
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ClientSession::DoUpdate()
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
  }
}

//------------------------------------------------------------------------------
// Handshake response
//------------------------------------------------------------------------------
void ClientSession::OnHandshake(const SessionPktSP& pkt)
{
  if (SESSION_STATE_HANDSHAKING != m_sess_state) {
    LOG_DBG("Ignore handshake response");
    return;
  }

	HandshakeData data = SessionProtocol::ParseHandshakeData(pkt->buf);

	LOG_INF("[session:{}] Received handshake response, rsid:{}, ver:{}, fec:{}, "
		"session:{}, kai:{}, service:{}, peer:{}",
		m_sess_param.local_sid,
		m_sess_param.remote_sid,
    (uint8_t)data.version,
    (uint8_t)data.fec_type,
    (uint8_t)data.session_type,
    (uint8_t)data.ka_interval, 
    data.service_type, 
    data.peer_ip)

	// Save the remote parameters
	m_sess_param.remote_sid = pkt->head.src;
  m_sess_param.remote_kai = data.ka_interval;
  m_sess_param.peer_seen_ip = data.peer_ip;
  m_sess_param.fec_type = (com::FecType)data.fec_type;

  OnSessionNegotiateCompelete();
	NotifySessionCreateResult(true);

	m_sess_state = SESSION_STATE_TRANSPORTING;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ClientSession::OnReconnectRsp(const SessionPktSP& sp)
{
	LOG_INF("[session:{}] Received reconnect response, rsid:{}", 
		m_sess_param.local_sid, m_sess_param.remote_sid);

	m_sess_state = SESSION_STATE_TRANSPORTING;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ClientSession::OnSessionClose(const SessionPktSP& sp)
{
	LOG_INF("[session:{}] Received session close, rsid:{}", 
		m_sess_param.local_sid, m_sess_param.remote_sid);

	m_sess_mgr->InnerCloseSession(m_sess_param.local_sid);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ClientSession::OnKeepAlive(const SessionPktSP& sp)
{
	LOG_INF("[session:{}] Received keep alive message, rsid:{}", 
		m_sess_param.local_sid, m_sess_param.remote_sid);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ClientSession::OnSessionControlMsg(const SessionPktSP& pkt)
{
	switch (pkt->head.pt) {
	case SESSION_PKT_HANDSHAKE:
		OnHandshake(pkt);
		break;
	case SESSION_PKT_KEEP_ALIVE:
		OnKeepAlive(pkt);
		break;
	case SESSION_PKT_RECONNECT_RSP:
		OnReconnectRsp(pkt);
		break;
	case SESSION_PKT_CLOSE:
		OnSessionClose(pkt);
		break;
	default:
		LOG_ERR("Unexpected message type {}", (uint8_t)pkt->head.pt);
		break;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ClientSession::DoClose()
{
  if (m_sess_param.remote_addr.type == com::AddrType::UDP) {
    m_sess_mgr->m_udp_mgr->CloseSocket(m_sess_param.sock);
  }
  else if (m_sess_param.remote_addr.type == com::AddrType::TCP) {
    m_sess_mgr->m_tcp_mgr->CloseConn(m_sess_param.sock);
  }
}

}