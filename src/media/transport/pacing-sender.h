#pragma once

#include <list>

#include "common-struct.h"
#include "com-factory.h"
#include "if-timer-mgr.h"

namespace jukey::txp
{

//==============================================================================
// 
//==============================================================================
class IPacingSenderHandler
{
public:
	virtual void OnPacingData(const com::Buffer& buf) = 0;
};

//==============================================================================
// 
//==============================================================================
enum DataPriroity
{
	DP_LOW,
	DP_HIGH
};

//==============================================================================
// 
//==============================================================================
class PacingSender
{
public:
	PacingSender(base::IComFactory* factory, IPacingSenderHandler* handler);
	~PacingSender();

	void EnqueueData(const com::Buffer& buf, DataPriroity priority);
	void SetPacingRate(uint32_t rate_kbps);
	void SetPacingFactor(double factor);
	void OnProbeSent(uint32_t size);

private:
	void OnTimer();
	void RefreshBitrateStats(const com::Buffer& buf);
	void UpdateIntervalSendBytes();

private:
	struct StatsEntry
	{
		StatsEntry(uint64_t t, uint32_t l) : ts(t), data_len(l) {}

		uint64_t ts;
		uint32_t data_len;
	};

private:
	base::IComFactory* m_factory = nullptr;
	IPacingSenderHandler* m_handler = nullptr;

	com::ITimerMgr* m_timer_mgr = nullptr;
	com::TimerId m_timer_id = INVALID_TIMER_ID;

	// Priority packet list
	std::list<com::Buffer> m_low_pri_list;
	std::list<com::Buffer> m_high_pri_list;

	// Send bitrate stats
	std::list<StatsEntry> m_stats_list;
	uint64_t m_stats_bytes = 0;

	static const uint32_t kMaxStatsDurationMs = 400;
	static const uint32_t kSendIntervalMs = 5;
	static const uint32_t kUpdateInnerPacingRateThreshold = 16;
	static const double kInnerPacingFactorMultiply;
	static const uint32_t kMaxContOverflowTimes = 3;

	// Bytes to send for next period
	uint64_t m_interval_send_bytes = 0;

	std::mutex m_mutex;

	uint32_t m_pacing_rate_kbps = 0;
	double m_pacing_factor = 2.0;
	double m_inner_pacing_factor = 1.0;

	uint64_t m_last_dump_us = 0;

	uint32_t m_empty_times = 0;

	uint32_t m_probe_sent_size = 0;

	// 连续发送后还剩余报文的次数
	uint32_t m_consecutive_overflows = 0;
};

}