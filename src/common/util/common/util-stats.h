#pragma once

#include <string>
#include <unordered_map>
#include <mutex>

#include "com-factory.h"
#include "if-timer-mgr.h"
#include "log/spdlog-wrapper.h"

namespace jukey::util
{

typedef uint32_t StatsId;

#define INVALID_STATS_ID 0

//==============================================================================
// 
//==============================================================================
enum StatsType
{
	INVALID = 0,
	IACCU = 1, // Interval Accumulate
	IAVER = 2, // Interval Accumulate / Interval Time(sencond)
	ISNAP = 3, // Interval Lastest Snapshot
	TACCU = 4, // Total Accumulate
	ICAVG = 5, // Interval Accumulate / Interval Item Count
};

//==============================================================================
// 
//==============================================================================
struct StatsParam
{
	StatsParam() {}

	StatsParam(const std::string& n, StatsType t, uint32_t i)
		: name(n), stats_type(t), interval(i) {}

	std::string name;
	std::string unit;
	StatsType   stats_type = INVALID;
	uint32_t    interval = 0;
	uint32_t    mul_factor = 0;
	uint32_t    div_factor = 0;
};

//==============================================================================
// 
//==============================================================================
class DataStats : public std::enable_shared_from_this<DataStats>
{
public:
	DataStats(base::IComFactory* factory, 
		SpdlogWrapperSP logger,
		const std::string& log_prefix,
		bool log_repeat = true);

	~DataStats();

	void Start();
	void Stop();

	StatsId AddStats(const StatsParam& param);

	void RemoveStats(StatsId stats_id);
	
	void OnData(StatsId stats_id, uint32_t stats_data);

	void OnTimeout();

private:
	struct StatsNode
	{
		StatsParam param;

		uint64_t start_time = 0;
		uint64_t last_stats_time = 0;
		uint64_t interval_total_data = 0;
		uint64_t origin_total_data = 0;
		uint64_t interval_data_count = 0;
	};

private:
	std::unordered_map<StatsId, StatsNode> m_stats_map;
	com::TimerId m_timer_id = INVALID_TIMER_ID;
	std::mutex m_mutex;
	base::IComFactory* m_factory = nullptr;

	std::string m_log_prefix;

	bool m_log_repeat = true;

	// Check repeat log
	std::string m_last_log_str;

	uint32_t m_stats_alloc_index = 0;

	SpdlogWrapperSP m_logger;
};
typedef std::shared_ptr<DataStats> DataStatsSP;

}