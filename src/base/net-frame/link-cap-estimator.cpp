#include <algorithm>
#include <math.h>

#include "link-cap-estimator.h"
#include "common/util-time.h"


namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void LinkCapEstimator::OnProbe1Arraval()
{
	m_probe1_arrival_ts = util::Now();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void LinkCapEstimator::OnProbe2Arraval()
{
	int64_t delay = util::Now() - m_probe1_arrival_ts;

	if (m_arrival_delays.size() <= m_next_delay_index) {
		m_arrival_delays.push_back(delay);
	}
	else {
		m_arrival_delays[m_next_delay_index] = delay;
	}

	++m_next_delay_index;
	if (m_next_delay_index == s_max_delay_size) {
		m_next_delay_index = 0;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t LinkCapEstimator::GetLinkCap()
{
	if (m_arrival_delays.empty()) return 12; // TODO:

	std::vector<int64_t> delays = m_arrival_delays;
	std::nth_element(delays.begin(), delays.begin() + delays.size() / 2,
		delays.end());

	int64_t median = delays[delays.size() / 2];

	uint64_t count = 1;
	int64_t sum = median;
	int64_t upper = median << 3;
	int64_t lower = median >> 3;

	for (auto delay : delays) {
		if (delay < upper && delay > lower) {
			sum += delay;
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