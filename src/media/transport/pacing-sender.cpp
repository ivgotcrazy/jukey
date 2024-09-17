#include <cassert>

#include "pacing-sender.h"
#include "common/util-time.h"
#include "log.h"


using namespace jukey::base;

namespace jukey::txp
{

const double PacingSender::kInnerPacingFactorMultiply = 1.2;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
PacingSender::PacingSender(IComFactory* factory, IPacingSenderHandler* handler) 
	: m_factory(factory), m_handler(handler)
{
	m_timer_mgr = QUERY_TIMER_MGR(m_factory);
	assert(m_timer_mgr);

	com::TimerParam timer_param;
	timer_param.timeout = kSendIntervalMs;
	timer_param.timer_type = com::TimerType::TIMER_TYPE_LOOP;
	timer_param.timer_name = "pacing sender";
	timer_param.timer_func = [this](int64_t) { OnTimer(); };

	m_timer_id = m_timer_mgr->AllocTimer(timer_param);
	m_timer_mgr->StartTimer(m_timer_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
PacingSender::~PacingSender()
{
	m_timer_mgr->StopTimer(m_timer_id);
	m_timer_mgr->FreeTimer(m_timer_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void PacingSender::RefreshBitrateStats(const com::Buffer& buf)
{
	uint64_t now = util::Now();

	while (!m_stats_list.empty()) {
		const auto& item = m_stats_list.front();

		// 移除超过统计范围的报文
		if (now < item.ts + kMaxStatsDurationMs * 1000) {
			break;
		}

		if (m_stats_bytes >= item.data_len) {
			m_stats_bytes -= item.data_len;
		}
		else {
			LOG_ERR("Invalid stats bytes:{}, len:{}", m_stats_bytes, item.data_len);
			m_stats_bytes = 0;
		}

		m_stats_list.pop_front();
	}

	m_stats_list.push_back(StatsEntry(now, buf.data_len));
	m_stats_bytes += buf.data_len;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void PacingSender::EnqueueData(const com::Buffer& buf, DataPriroity priority)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	// 初始状态，还未计算出每个时间间隔发送的数据大小，直接发送
	if (m_interval_send_bytes == 0) {
		m_handler->OnPacingData(buf);
	}
	else { // 根据优先级入发送队列
		if (priority == DP_LOW) {
			m_low_pri_list.push_back(buf);
		}
		else if (priority == DP_HIGH) {
			m_high_pri_list.push_back(buf);
		}
	}

	RefreshBitrateStats(buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void PacingSender::SetPacingRate(uint32_t rate_kbps)
{
	LOG_INF("Set pacing rate from {} to {}", m_pacing_rate_kbps, rate_kbps);

	m_pacing_rate_kbps = rate_kbps;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void PacingSender::SetPacingFactor(double factor)
{
	if (factor < 0.1) {
		LOG_ERR("Invalid pacing fator:{}", factor);
		return;
	}

	LOG_INF("Set pacing factor from {} to {}", m_pacing_factor, factor);

	m_pacing_factor = factor;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void PacingSender::OnProbeSent(uint32_t size)
{
	m_probe_sent_size += size;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void PacingSender::UpdateIntervalSendBytes()
{
	// 没有发送报文
	if (m_stats_list.empty()) {
		m_stats_bytes = 0;
		m_interval_send_bytes = 0;
		return;
	}

	if (m_pacing_rate_kbps == 0) {
		uint64_t diff_us = (util::Now() - m_stats_list.front().ts);
		uint64_t bitrate = ((uint64_t)m_stats_bytes * 1000 * 1000 * 8) / diff_us;

		LOG_DBG("bitrate:{}, size:{}, bytes:{}, duration:{}", bitrate,
			m_stats_list.size(), m_stats_bytes, diff_us);

		uint64_t send_bytes = static_cast<uint64_t>(
			m_pacing_factor * 
			m_inner_pacing_factor * 
			m_stats_bytes * (kSendIntervalMs * 1000) / diff_us);

		// 平滑
		m_interval_send_bytes = (m_interval_send_bytes * 8 + send_bytes * 2) / 10;
	}
	else {
		m_interval_send_bytes = static_cast<uint32_t>(m_pacing_factor *
			m_inner_pacing_factor * m_pacing_rate_kbps * kSendIntervalMs / 8);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void PacingSender::OnTimer()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	UpdateIntervalSendBytes();
	
	if (m_interval_send_bytes == 0 
		|| m_interval_send_bytes <= m_probe_sent_size) {
		m_probe_sent_size = 0;
		return;
	}
		
	uint64_t need_send_bytes = m_interval_send_bytes - m_probe_sent_size;

	while (!m_high_pri_list.empty() || !m_low_pri_list.empty()) {
		if (need_send_bytes == 0)
			break;

		// 先发送高优先级报文
		if (!m_high_pri_list.empty()) {
			m_handler->OnPacingData(m_high_pri_list.front());
			if (m_high_pri_list.front().data_len >= need_send_bytes) {
				need_send_bytes = 0;
			}
			else {
				need_send_bytes -= m_high_pri_list.front().data_len;
			}
			m_high_pri_list.pop_front();
		}
		// 再发送低优先级报文
		else if (!m_low_pri_list.empty()) {
			m_handler->OnPacingData(m_low_pri_list.front());
			if (m_low_pri_list.front().data_len >= need_send_bytes) {
				need_send_bytes = 0;
			}
			else {
				need_send_bytes -= m_low_pri_list.front().data_len;
			}
			m_low_pri_list.pop_front();
		}
	}

	// Update 
	size_t left_size = m_low_pri_list.size() + m_high_pri_list.size();
	if (left_size == 0) {
		if (m_inner_pacing_factor > 1.0) {
			if (++m_empty_times >= 3) {
				m_inner_pacing_factor = 1.0;
				LOG_INF("Restore inner pacing factor to {}, pacing factor:{}, pacing rate:{}", 
					m_inner_pacing_factor, m_pacing_factor, m_pacing_rate_kbps);
			}
		}
		m_consecutive_overflows = 0;
	} else if (left_size > kUpdateInnerPacingRateThreshold) {
		if (++m_consecutive_overflows >= kMaxContOverflowTimes) {
			m_inner_pacing_factor *= kInnerPacingFactorMultiply;
			LOG_INF("Update inner pacing factor to {}, low size:{}, high size:{}, "
				"pacing factor:{}, pacing rate:{}",
				m_inner_pacing_factor, m_low_pri_list.size(), m_high_pri_list.size(),
				m_pacing_factor, m_pacing_rate_kbps);
		}
		m_empty_times = 0;
	}
	else if (left_size > 0 && left_size <= kUpdateInnerPacingRateThreshold) {
		m_empty_times = 0;
		m_consecutive_overflows = 0;
	}

	if (util::Now() > m_last_dump_us + 1000 * 1000 * 5) {
		LOG_INF("Pacing rate:{}, pacing factor:{}, low size:{}, high size:{}", 
			m_pacing_rate_kbps, m_pacing_factor, m_low_pri_list.size(), 
			m_high_pri_list.size());
		m_last_dump_us = util::Now();
	}
}

}