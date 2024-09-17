#include "rtt-filter.h"
#include "net-common.h"
#include "common-config.h"
#include "log.h"


namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
RttFilter::RttFilter(SessionId local_sid) : m_local_sid(local_sid)
{

}

//------------------------------------------------------------------------------
// Caculate weighted mean RTT and smooth variance
//------------------------------------------------------------------------------
uint32_t RttFilter::UpdateRtt(uint32_t rtt)
{
	if (m_srtt == 0) { // first time
		m_srtt = rtt;
		m_rtt_var = rtt / 2;
	}
	else {
		uint32_t delta = rtt > m_srtt ? (rtt - m_srtt) : (m_srtt - rtt);
		m_rtt_var = (3 * m_rtt_var + delta) / 4;
		m_srtt = (7 * m_srtt + rtt) / 8;
		if (m_srtt < 1) {
			m_srtt = 1;
		}
	}

	LOG_DBG("[session:{}] rtt:{}, srtt:{}, rtt-var:{}", m_local_sid, rtt, m_srtt,
		m_rtt_var);

	return m_srtt;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t RttFilter::GetRto()
{
	uint32_t old_rto = m_rto;

	// Update RTO
	m_rto = LowboundAndUpbound(m_srtt + 4 * m_rtt_var + SESSION_UPDATE_INTERVAL,
		MIN_RTO, MAX_RTO);

	if (old_rto != m_rto) {
		LOG_INF("[session:{}] old rto:{}, new rto:{}", m_local_sid, old_rto, m_rto);
	}

	return m_rto;
}

}