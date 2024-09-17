#include "recv-pkt-tracer.h"
#include "common/util-time.h"


using namespace jukey::util;

namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
RecvPktTracer::RecvPktTracer(uint64_t duration) : m_max_duration(duration)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RecvPktTracer::AddPktCount(uint32_t count, uint64_t duration)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_pkt_count_list.push_back(PktCountItem(count, Now(), duration));

	m_list_size++;
	m_total_duration += duration;
	m_total_pkt_count += count;

	ProcDuration();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t RecvPktTracer::GetPktCount()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	ProcDuration();

	return m_total_pkt_count;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RecvPktTracer::ProcDuration()
{
	uint64_t now = Now();

	for (auto iter = m_pkt_count_list.begin(); iter != m_pkt_count_list.end();) {
		if (iter->ts + m_max_duration <= now) {
			m_list_size--;
			m_total_duration -= iter->duration;
			m_total_pkt_count -= iter->count;

			iter = m_pkt_count_list.erase(iter);
		}
		else {
			break;
		}
	}
}

}