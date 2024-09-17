#include <sstream>

#include "session-base.h"
#include "net-inner-message.h"
#include "session-mgr.h"
#include "common/util-time.h"
#include "tcp-session-pkt-assembler.h"
#include "rtt-filter.h"
#include "fec-pkt-assembler.h"
#include "log.h"

using namespace jukey::util;
using namespace jukey::com;

namespace jukey::net
{
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionBase::SessionBase(SessionMgr* mgr, SessionThreadSP st, 
  const SessionParam& param)
	: m_sess_mgr(mgr)
	, m_factory(mgr->GetComFactory())
	, m_sess_thread(st)
	, m_sess_param(param)
	, m_sess_state(SESSION_STATE_INITIALIZED)
{
	
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionBase::~SessionBase()
{
	LOG_DBG("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionBase::CreateObjects()
{
	// Assembler tcp session packets
	if (m_sess_param.remote_addr.type == com::AddrType::TCP) {
		m_sess_pkt_assembler.reset(new TcpSessionPktAssembler(
			m_sess_param.local_sid));
	}

	// Send with congestion control
	m_pkt_sender.reset(new SessionPktSender(m_factory, m_sess_mgr->m_tcp_mgr,
		m_sess_mgr->m_udp_mgr, this, m_sess_param));

	// Calculate link RTT dynamically
	m_rtt_filter.reset(new RttFilter(m_sess_param.local_sid));

	// Estimate link capacity dynamically
	m_link_cap_estimator.reset(new LinkCapEstimator());

	// Assembler FEC packets
	m_fec_pkt_assembler.reset(new FecPktAssembler(m_factory, m_sess_param));

	m_sess_recver.reset(new SessionReceiver(m_factory, 
		m_sess_param,
		m_pkt_sender, 
		m_rtt_filter, 
		m_link_cap_estimator, 
		m_fec_pkt_assembler));

	m_sess_sender.reset(new SessionSender(m_factory, 
		m_sess_param,
		m_pkt_sender,
		m_rtt_filter,
		m_link_cap_estimator));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionBase::Init()
{
	CreateObjects();

	m_last_send_data_ts = m_last_recv_data_ts = util::Now();

	DoInit(); // derived class process

	m_sess_state = SESSION_STATE_HANDSHAKING;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionBase::SendHandshake(const HandshakeData& data)
{
	Buffer buf = SessionProtocol::BuildHandshakePkt(m_sess_param.local_sid,
		m_sess_param.remote_sid, data);

	m_pkt_sender->SendPkt(SESSION_PKT_HANDSHAKE, buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionBase::SendKeepAlive()
{
	Buffer buf = SessionProtocol::BuildKeepAlivePkt(m_sess_param.local_sid,
		m_sess_param.remote_sid);

	LOG_DBG("[session:{}] Send keep alive message", m_sess_param.local_sid);

	m_pkt_sender->SendPkt(SESSION_PKT_KEEP_ALIVE, buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionBase::SendSessionClose()
{
	Buffer buf = SessionProtocol::BuildSessionClosePkt(m_sess_param.local_sid,
		m_sess_param.remote_sid);

  LOG_INF("[session:{}] Send session close message", m_sess_param.local_sid);

	m_pkt_sender->SendPkt(SESSION_PKT_CLOSE, buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionBase::SendReconnectReq()
{
  Buffer buf = SessionProtocol::BuildReconnectReqPkt(m_sess_param.local_sid,
    m_sess_param.remote_sid);

  LOG_INF("[session:{}] Send reconnect request", m_sess_param.local_sid);

  m_pkt_sender->SendPkt(SESSION_PKT_RECONNECT_REQ, buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionBase::SendReconnectRsp()
{
  Buffer buf = SessionProtocol::BuildReconnectRspPkt(m_sess_param.local_sid,
    m_sess_param.remote_sid);

  LOG_INF("[session:{}] Send reconnect response", m_sess_param.local_sid);

  m_pkt_sender->SendPkt(SESSION_PKT_RECONNECT_RSP, buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionBase::OnUpdate()
{
  m_sess_recver->OnUpdate();
  m_sess_sender->OnUpdate();
  m_fec_pkt_assembler->Update();

  DoUpdate(); // derived class process
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
const SessionParam& SessionBase::GetParam()
{
  return m_sess_param;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint64_t SessionBase::GetEntryId()
{ 
  return m_sess_param.local_sid; 
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint64_t SessionBase::NextSendTime()
{ 
  if (m_sess_state != SESSION_STATE_CLOSED) {
    return m_sess_sender->NextSendTime();
  }
  else {
    return INVALID_SEND_TIME;
  }
}

//------------------------------------------------------------------------------
// Send data to network
//------------------------------------------------------------------------------
void SessionBase::SendData() 
{ 
  m_sess_sender->SendSessionData(); 
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionBase::OnSessionNegotiateCompelete()
{
  m_sess_sender->OnSessionNegotiateComplete();
}

//------------------------------------------------------------------------------
// Post data to app
//------------------------------------------------------------------------------
void SessionBase::OnSessionData(const SessionPktSP& pkt)
{
	LOG_DBG("[session:{}] Received session data, len:{}, msn:{}, psn:{}", 
    m_sess_param.local_sid, pkt->buf.data_len, pkt->head.msn, pkt->head.psn);

	SessionDataMsgSP data(new SessionDataMsg(
		m_sess_param.session_role, 
		m_sess_param.local_sid,
		m_sess_param.remote_sid,
		pkt->buf));

	com::CommonMsg msg;
	msg.msg_type = NET_MSG_SESSION_DATA;
	msg.msg_data = data;
	m_sess_param.thread->PostMsg(msg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionBase::OnSendPkt(uint32_t data_type, uint32_t data_len)
{
  m_last_send_data_ts = util::Now();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionBase::Close(bool active)
{
	if (m_sess_state == SESSION_STATE_CLOSED) {
		LOG_INF("[session:{}] Session already closed, remote session:{}",
			m_sess_param.local_sid, m_sess_param.remote_sid);
		return;
	}

	LOG_INF("[session:{}] Close session, remote session:{}",
		m_sess_param.local_sid, m_sess_param.remote_sid);

	// 向对端发送关闭消息
	SendSessionClose();

	DoClose(); // derived class process

	m_sess_state = SESSION_STATE_CLOSED;

	if (!active) {
		com::CommonMsg msg;
		msg.msg_type = NET_MSG_SESSION_CLOSED;
		msg.msg_data.reset(new SessionClosedMsg(m_sess_param.session_role,
			m_sess_param.local_sid, m_sess_param.remote_sid));

		// Notify application
		m_sess_param.thread->PostMsg(msg);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionBase::ProcSessionDataPkt(const SessionPktSP& pkt)
{
	if (m_sess_state != SessionState::SESSION_STATE_TRANSPORTING) {
		LOG_WRN("[session:{}] Received session data in state:{}, len:{}", 
			m_sess_param.local_sid, m_sess_state, pkt->buf.data_len);
		return;
	}

	m_sess_recver->OnSessionData(pkt);
	while (SessionPktSP data = m_sess_recver->GetSessionMsg()) {
		SessionBase::OnSessionData(data);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionBase::OnRecvSessionPkt(const SessionPktSP& pkt)
{
	if (!pkt) return;

	m_last_recv_data_ts = util::Now();

	if (pkt->head.pt != SESSION_PKT_DATA) {
		SessionProtocol::DumpSessionPkt(m_sess_param.local_sid, pkt);
	}

	if (m_last_recv_psn != 0 && pkt->head.psn > m_last_recv_psn + 1) {
		LOG_DBG("Lost sessiond data, last psn:{}, recv psn:{}",
			m_last_recv_psn, pkt->head.psn);
	}

	m_last_recv_psn = pkt->head.psn;

	LOG_DBG("[session:{}] Received session protocal data, type:{}, len:{}",
		m_sess_param.local_sid,
		(uint32_t)pkt->head.pt, 
		pkt->buf.data_len);

	switch (pkt->head.pt) {
	case SESSION_PKT_DATA:
		ProcSessionDataPkt(pkt);
		break;
	case SESSION_PKT_ACK:
		m_sess_sender->OnSessionAck(pkt);
		break;
	case SESSION_PKT_ACK2:
		m_sess_recver->OnSessionAck2(pkt);
		break;
	case SESSION_PKT_REPORT:
		m_sess_sender->OnSessionReport(pkt);
		break;
	case SESSION_PKT_HANDSHAKE:
	case SESSION_PKT_KEEP_ALIVE:
	case SESSION_PKT_RECONNECT_REQ:
	case SESSION_PKT_RECONNECT_RSP:
	case SESSION_PKT_CLOSE:
		OnSessionControlMsg(pkt);
		break;
	default:
		LOG_ERR("[session:{}] Unexpected message type {}", 
				m_sess_param.local_sid, (uint8_t)pkt->head.pt);
		break;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionBase::OnRecvFecProtocolData(const com::Buffer& buf)
{
  if (!m_fec_pkt_assembler) {
    LOG_ERR("[session:{}] Invalid fec packet assembler!", m_sess_param.local_sid);
    return;
  }

  m_fec_pkt_assembler->InputFecData(buf);

  com::Buffer fec_buf;
  while (m_fec_pkt_assembler->GetNextSourceData(fec_buf)) {
    OnRecvSessionPkt(ConvertSessionPkt(fec_buf));
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionPktSP SessionBase::ConvertSessionPkt(const com::Buffer& buf)
{
	if (!CheckBuffer(buf)) return nullptr;

	SesPktHdr* ses_hdr = nullptr;

	if ((*buf.data.get() & 0x3) == 0) { // session packet
		ses_hdr = (SesPktHdr*)buf.data.get();
	}
	else { // fec packet
		ses_hdr = (SesPktHdr*)(buf.data.get() + FEC_PKT_HDR_LEN);
	}

	SessionPktSP pkt(new SessionPkt(*ses_hdr, buf));
	pkt->buf.data_len = ses_hdr->len;

	if ((*buf.data.get() & 0x3) == 0) { // session packet
		pkt->buf.start_pos = SES_PKT_HDR_LEN;
	}
	else { // fec packet
		pkt->buf.start_pos = SES_PKT_HDR_LEN + FEC_PKT_HDR_LEN;
	}

	pkt->ts = util::Now();

	return pkt;
}

//------------------------------------------------------------------------------
// Receive data from network
//------------------------------------------------------------------------------
void SessionBase::OnRecvData(const com::Buffer& buf)
{
	if (m_sess_param.remote_addr.type == AddrType::TCP) {
		m_sess_pkt_assembler->InputSessionData(buf);
		while (SessionPktSP pkt = m_sess_pkt_assembler->GetNextSessionPkt()) {
			OnRecvSessionPkt(pkt);
		}
	}
	else if (m_sess_param.remote_addr.type == AddrType::UDP) {
		if ((*(buf.data.get()) & 0x3) == 0) { // session protocol
			LOG_DBG("[session:{}] Received session protocol data, len:{}",
				m_sess_param.local_sid, buf.data_len);
			OnRecvSessionPkt(ConvertSessionPkt(buf));
		}
		else if ((*(buf.data.get()) & 0x3) == 1) { // fec protocol
			LOG_DBG("[session:{}] Received fec protocol data, len:{}",
				m_sess_param.local_sid, buf.data_len);
			OnRecvFecProtocolData(buf);
		}
		else {
			LOG_ERR("Unknown protocol flag:{}", (*(buf.data.get()) & 0x3));
		}
	}
	else {
		LOG_ERR("Invalid address type:{}", m_sess_param.remote_addr.type);
	}
}

//------------------------------------------------------------------------------
// App send data
//------------------------------------------------------------------------------
bool SessionBase::OnSendData(const com::Buffer& buf)
{
  if (m_sess_state == SESSION_STATE_TRANSPORTING) {
    return m_sess_sender->PushSessionData(buf);
  }
  else {
    LOG_ERR("[session:{}] Invalid session state {}", m_sess_param.local_sid,
      m_sess_state);
    return false;
  }
}

}