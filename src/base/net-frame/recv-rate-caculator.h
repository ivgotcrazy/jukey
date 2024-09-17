#pragma once

#include <inttypes.h>
#include <vector>

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class RecvRateCaculator
{
public:
	RecvRateCaculator();

	void OnPktArrival(uint32_t pkt_size);

	uint32_t GetPktRate();

private:
	struct PktItem
	{
		uint64_t interval;
		uint32_t pkt_size;
	};

	struct PktItemCmp
	{
		bool operator()(const PktItem& lhs, const PktItem& rhs)
		{
			return lhs.interval < rhs.interval;
		}
	};

	static const uint32_t s_max_pkt_size = 16;

private:
	std::vector<PktItem> m_arrival_pkts;
	uint32_t m_next_pkt_index = 0;
	uint64_t m_last_pkt_arrival_ts = 0;
};

}