#include "session-pkt-sender.h"
#include "fec-protocol.h"
#include "log.h"

using namespace jukey::com;
using namespace jukey::util;

namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionPktSender::SessionPktSender(base::IComFactory* factory, ITcpMgr* tcp_mgr,
	IUdpMgr* udp_mgr, ISendPktNotify* notify, const SessionParam& param)
	: m_tcp_mgr(tcp_mgr)
	, m_udp_mgr(udp_mgr)
	, m_sess_param(param)
	, m_notify(notify)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode SessionPktSender::SendPkt(uint32_t data_type, const com::Buffer& buf)
{
	ErrCode ec = ERR_CODE_FAILED;

	if (data_type == SESSION_PKT_DATA) {
		SesPktHdr* ses_hdr = (SesPktHdr*)(buf.data.get() + buf.start_pos);
		SessionProtocol::DumpSessionPktHdr(m_sess_param.local_sid, *ses_hdr);
	}
	else if (data_type == FEC_PKT_TYPE) {
		FecPktHdr* fec_hdr = (FecPktHdr*)buf.data.get();
		FecProtocol::DumpFecPktHdr(*fec_hdr);

		SesPktHdr* ses_hdr = (SesPktHdr*)(buf.data.get() + FEC_PKT_HDR_LEN);
		SessionProtocol::DumpSessionPktHdr(m_sess_param.local_sid, *ses_hdr);
	}

	if (m_sess_param.remote_addr.type == com::AddrType::UDP) {
		if (ERR_CODE_OK != m_udp_mgr->SendData(m_sess_param.sock,
			m_sess_param.remote_addr.ep, buf)) {
			LOG_ERR("[session:{}] Send session packet:{} failed!",
				m_sess_param.local_sid, data_type);
			return ERR_CODE_FAILED;
		}
	}
	else if (m_sess_param.remote_addr.type == com::AddrType::TCP) {
		if (ERR_CODE_OK != m_tcp_mgr->SendData(m_sess_param.sock, buf)) {
			LOG_ERR("[session:{}] Send session packet:{} failed!",
				m_sess_param.local_sid, data_type);
			return ERR_CODE_FAILED;
		}
	}
	else {
		LOG_ERR("Invalid address type:{}", m_sess_param.remote_addr.type);
		return ERR_CODE_FAILED;
	}

	LOG_DBG("[session:{}] Send session packet:{} success",
		m_sess_param.local_sid, data_type);

	if (m_notify) m_notify->OnSendPkt(data_type, buf.data_len);

	return ERR_CODE_OK;
}

}