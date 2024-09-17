#pragma once

#include <inttypes.h>
#include <list>
#include <mutex>
#include <memory>

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class RecvPktTracer
{
public:
	RecvPktTracer(uint64_t duration);

	void AddPktCount(uint32_t count, uint64_t duration);

	uint32_t GetPktCount();

private:
	void ProcDuration();

private:
	struct PktCountItem
	{
		PktCountItem() {}
		PktCountItem(uint32_t c, uint64_t t, uint64_t d) 
			: count(c), ts(t), duration(d) {}

		uint32_t count = 0;
		uint64_t ts = 0;
		uint64_t duration = 0;
	};

private:
	uint64_t m_max_duration = 0;

	std::list<PktCountItem> m_pkt_count_list;
	uint32_t m_list_size = 0;
	std::mutex m_mutex;

	uint64_t m_total_duration = 0;
	uint32_t m_total_pkt_count = 0;
};
typedef std::unique_ptr<RecvPktTracer> RecvPktTracerUP;

}