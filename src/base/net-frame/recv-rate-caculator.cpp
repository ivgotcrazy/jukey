#include <algorithm>
#include <math.h>

#include "recv-rate-caculator.h"
#include "common/util-time.h"


namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
RecvRateCaculator::RecvRateCaculator()
{
	m_last_pkt_arrival_ts = util::Now();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RecvRateCaculator::OnPktArrival(uint32_t pkt_size)
{
	uint64_t now = util::Now();

	PktItem pkt_item;
	pkt_item.interval = now - m_last_pkt_arrival_ts;
	pkt_item.pkt_size = pkt_size;

	if (m_arrival_pkts.size() <= m_next_pkt_index) {
		m_arrival_pkts.push_back(pkt_item);
	}
	else {
		m_arrival_pkts[m_next_pkt_index] = pkt_item;
	}

	++m_next_pkt_index;
	if (m_next_pkt_index == s_max_pkt_size) {
		m_next_pkt_index = 0;
	}

	m_last_pkt_arrival_ts = now;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t RecvRateCaculator::GetPktRate()
{
	if (m_arrival_pkts.empty()) return 0;

	std::vector<PktItem> pkts = m_arrival_pkts;

	std::nth_element(pkts.begin(), pkts.begin() + pkts.size() / 2, pkts.end(),
		PktItemCmp());

	uint64_t median = pkts[pkts.size() / 2].interval;

	uint64_t count = 0;
	uint64_t sum = 0;
	uint64_t upper = median << 3;
	uint64_t lower = median >> 3;

	for (auto pkt : pkts) {
		if (pkt.interval < upper && pkt.interval > lower) {
			sum += pkt.interval;
			++count;
		}
	}

	if (sum == 0 || count == 0) {
		return 0;
	}
	else {
		return (uint32_t)(1000000 * count / sum);
	}
}

}