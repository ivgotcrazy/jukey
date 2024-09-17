#include <iostream>

#include "util-stats.h"
#include "log/util-log.h"
#include "common/util-time.h"



////////////////////////////////////////////////////////////////////////////////
// Use external logger

#define LOG_DBG(...)                                            \
  if (m_logger && m_logger->GetLogger()) {                      \
    SPDLOG_LOGGER_DEBUG(m_logger->GetLogger(), __VA_ARGS__);    \
  }

#define LOG_INF(...)                                            \
  if (m_logger && m_logger->GetLogger()) {                      \
    SPDLOG_LOGGER_INFO(m_logger->GetLogger(), __VA_ARGS__);     \
  }

#define LOG_WRN(...)                                            \
  if (m_logger && m_logger->GetLogger()) {                      \
    SPDLOG_LOGGER_WARN(m_logger->GetLogger(), __VA_ARGS__);     \
  }

#define LOG_ERR(...)                                            \
  if (m_logger && m_logger->GetLogger()) {                      \
    SPDLOG_LOGGER_ERROR(m_logger->GetLogger(), __VA_ARGS__);    \
  }

#define LOG_CRT(...)                                            \
  if (m_logger && m_logger->GetLogger()) {                      \
    SPDLOG_LOGGER_CRITICAL(m_logger->GetLogger(), __VA_ARGS__); \
  }
////////////////////////////////////////////////////////////////////////////////


namespace jukey::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
DataStats::DataStats(base::IComFactory* factory, SpdlogWrapperSP logger,
	const std::string& log_prefix, bool log_repeat) 
	: m_factory(factory)
	, m_logger(logger)
	, m_log_prefix(log_prefix)
  , m_log_repeat(log_repeat)
{
	
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
DataStats::~DataStats()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_timer_id != INVALID_TIMER_ID) {
		com::ITimerMgr* timer_mgr = QUERY_TIMER_MGR(m_factory);
		assert(timer_mgr);
		
		timer_mgr->StopTimer(m_timer_id);
		timer_mgr->FreeTimer(m_timer_id);

		m_timer_id = INVALID_TIMER_ID;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void DataStats::OnTimeout()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	uint64_t now = util::Now();
	uint64_t diff = 0;
	std::string stats_str;

	for (auto& [id, node] : m_stats_map) {
		// The time has not arrived yet
		if (node.last_stats_time + node.param.interval * 1000 > now) {
			continue;
		}

		if (stats_str.empty()) {
			stats_str.append(m_log_prefix).append(" ");
		}
		else {
			stats_str.append(", ");
		}

		stats_str.append(node.param.name).append(":[");

		if (node.param.stats_type == IAVER) { // per second
			diff = node.interval_total_data * 1000 / node.param.interval;
		}
		else if (node.param.stats_type == IACCU) {
			diff = node.interval_total_data;
		}
		else if (node.param.stats_type == TACCU) {
			diff = node.origin_total_data;
		}
		else if (node.param.stats_type == ISNAP) {
			diff = node.interval_total_data;
		}
		else if (node.param.stats_type == ICAVG) {
			if (node.interval_data_count != 0) {
				diff = node.interval_total_data / node.interval_data_count;
			}
			else {
				diff = 0;
			}
		}
		else {
			continue;
		}

		if (node.param.mul_factor != 0)
			diff *= node.param.mul_factor;

		if (node.param.div_factor != 0)
			diff /= node.param.div_factor;

		if (node.param.unit.empty()) {
			stats_str.append(std::to_string(diff));
		}
		else {
			stats_str.append(std::to_string(diff)).append(node.param.unit);
		}

		stats_str.append("]");
			
		node.interval_total_data = 0;
		node.last_stats_time = now;
		node.interval_data_count = 0;
	}

	if (!stats_str.empty()) {
		if (m_log_repeat || (!m_log_repeat && m_last_log_str != stats_str)) {
			LOG_INF("{}", stats_str);
		}

		if (!m_log_repeat) {
			m_last_log_str = stats_str;
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void DataStats::Start()
{
	if (!m_factory) {
		return;
	}

	com::ITimerMgr* timer_mgr = QUERY_TIMER_MGR(m_factory);
	assert(timer_mgr);

	com::TimerParam timer_param;
	timer_param.timer_type  = com::TimerType::TIMER_TYPE_LOOP;
	timer_param.timeout     = 1000;
	timer_param.timer_name = "data stats";
	timer_param.run_atonce = false;
	timer_param.timer_func = [this](int64_t) { OnTimeout(); };
	
	m_timer_id = timer_mgr->AllocTimer(timer_param);
	timer_mgr->StartTimer(m_timer_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void DataStats::Stop()
{
	com::ITimerMgr* timer_mgr = QUERY_TIMER_MGR(m_factory);
	if (timer_mgr && m_timer_id != INVALID_TIMER_ID) {
		timer_mgr->StopTimer(m_timer_id);
		timer_mgr->FreeTimer(m_timer_id);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
StatsId DataStats::AddStats(const StatsParam& param)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto item : m_stats_map) {
		if (item.second.param.name == param.name) {
			LOG_WRN("Stats:{} already exists!", param.name);
			return INVALID_STATS_ID;
		}
	}

	if (param.interval == 0) {
		LOG_ERR("Invalid stats interval!");
		return INVALID_STATS_ID;
	}

	StatsNode node;
	node.param = param;
	node.start_time = util::Now();
	node.last_stats_time = node.start_time;

  // TODO: revert
	m_stats_map.insert(std::make_pair(++m_stats_alloc_index, node));

	return m_stats_alloc_index;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void DataStats::RemoveStats(StatsId stats_id)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_stats_map.end() == m_stats_map.find(stats_id)) {
		LOG_WRN("Stats:{} does not exist!", stats_id);
	}
	else {
		m_stats_map.erase(stats_id);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void DataStats::OnData(StatsId stats_id, uint32_t stats_data)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_stats_map.find(stats_id);
	if (iter == m_stats_map.end()) {
		LOG_ERR("Cannot find stats:{}", stats_id);
	}
	else {
	if (iter->second.param.stats_type == ISNAP) {
	  iter->second.interval_total_data = stats_data;
	}
	else {
	  iter->second.interval_total_data += stats_data;
	  iter->second.origin_total_data += stats_data;
	}
	iter->second.interval_data_count++;
	}
}

}
