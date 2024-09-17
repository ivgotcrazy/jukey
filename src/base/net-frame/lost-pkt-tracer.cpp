#include "lost-pkt-tracer.h"
#include "common/util-time.h"
#include "log.h"


namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
LostPktTracer::LostPktTracer(uint64_t duration) : m_duration(duration)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void LostPktTracer::AddLostPkt(uint32_t sn, uint64_t ts)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_lost_pkts.empty()) {
		m_oldest_ts = ts;
	}

	m_lost_pkts.insert(std::make_pair(sn, ts));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void LostPktTracer::AddLostPkt(uint32_t sn)
{
	AddLostPkt(sn, util::Now());

	ProcDuration();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void LostPktTracer::AddLostPkt(uint32_t begin_sn, uint32_t end_sn)
{
	uint64_t now = util::Now();

	for (uint32_t i = begin_sn; i < end_sn; i++) {
		AddLostPkt(i, now);
	}

	ProcDuration();

	LOG_DBG("Add lost packet, begin:{}, end:{}, lost count:{}", begin_sn, end_sn,
		m_lost_pkts.size());
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void LostPktTracer::RemoveLostPkt(uint32_t sn)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_lost_pkts.erase(sn);

	ProcDuration();

	LOG_DBG("Remove lost packet, sn:{}, lost count:{}", sn, m_lost_pkts.size());
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
PktLostInfo LostPktTracer::GetInfo()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	PktLostInfo lost_info;

	uint32_t max_lost_block_len = 0;
	uint32_t lost_block_count = 0;
	uint32_t last_lost_sn = 0;
	uint32_t lost_block_begin_sn = 0;

	for (auto iter = m_lost_pkts.begin(); iter != m_lost_pkts.end(); iter++) {
		if (last_lost_sn + 1 != iter->first) {
			if (lost_block_begin_sn != 0) {
				lost_block_count++;
				auto lost_block_len = last_lost_sn - lost_block_begin_sn + 1;
				if (lost_block_len > max_lost_block_len) {
					max_lost_block_len = lost_block_len;
				}
			}
			last_lost_sn = iter->first;
			lost_block_begin_sn = iter->first;
		}
		else {
			last_lost_sn = iter->first;
		}

		if (auto temp = iter; ++temp == m_lost_pkts.end()) {
			lost_block_count++;
			auto lost_block_len = last_lost_sn - lost_block_begin_sn + 1;
			if (lost_block_len > max_lost_block_len) {
				max_lost_block_len = lost_block_len;
			}
		}
	}

	lost_info.lost_count = (uint32_t)m_lost_pkts.size();
	lost_info.max_cts_lost_count = max_lost_block_len;

	if (lost_block_count != 0) {
		lost_info.avg_cts_lost_count = (uint32_t)m_lost_pkts.size() / lost_block_count;
	}

	return lost_info;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void LostPktTracer::ProcDuration()
{
	uint64_t now = util::Now();

	if (m_oldest_ts + m_duration <= now) {
		for (auto iter = m_lost_pkts.begin(); iter != m_lost_pkts.end();) {
			if (iter->second + m_duration <= now) {
				iter = m_lost_pkts.erase(iter);
			}
			else {
				m_oldest_ts = iter->second;
				break;
			}
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void LostPktTracer::Update()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	ProcDuration();
}

}