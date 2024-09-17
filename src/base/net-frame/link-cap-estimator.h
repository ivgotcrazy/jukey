#pragma once

#include <inttypes.h>
#include <vector>
#include <memory>

namespace jukey::net
{

class LinkCapEstimator
{
public:
	void OnProbe1Arraval();
	void OnProbe2Arraval();

	uint32_t GetLinkCap();

private:
	static const uint32_t s_max_delay_size = 16;
	std::vector<int64_t> m_arrival_delays;
	uint32_t m_next_delay_index = 0;
	uint64_t m_probe1_arrival_ts = 0;
};
typedef std::shared_ptr<LinkCapEstimator> LinkCapEstimatorSP;

}