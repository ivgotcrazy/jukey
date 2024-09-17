#include "congestion-controller.h"
#include "common-config.h"
#include "log.h"


namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
CongestionController::CongestionController(const SessionParam& param)
	: m_sess_param(param)
{
	m_cc_param.cwnd = MIN_CWND;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
const CCParam& CongestionController::GetCCParam()
{
	return m_cc_param;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CongestionController::UpdateRtt(uint32_t rtt)
{
	m_rtt = rtt;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CongestionController::UpdateInflight(uint32_t inflight)
{
	m_inflight = inflight;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CongestionController::UpdateRemoteWnd(uint32_t wnd)
{
	m_remote_wnd = wnd;

	if (wnd == 0) {
		LOG_WRN("Remote window is full!");
	}

	m_cc_param.cwnd = LowboundAndUpbound(m_cc_param.cwnd, MIN_CWND, m_remote_wnd);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CongestionController::UpdateLinkCap(uint32_t pkt_rate)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CongestionController::OnRecvAck(uint32_t sn, uint32_t count)
{
	if (m_cc_param.cwnd < m_remote_wnd) {
		if (m_cc_param.cwnd < m_ssthresh) {
			m_cc_param.cwnd++;
		}
		else {
			m_cc_param.cwnd += count;
		}
	}

	LOG_DBG("[session:{}] cwnd:{}", m_sess_param.local_sid, m_cc_param.cwnd);
}

}