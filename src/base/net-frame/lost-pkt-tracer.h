#pragma once

#include <map>
#include <mutex>
#include <memory>
#include <inttypes.h>

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
struct PktLostInfo
{
	uint32_t lost_count = 0;
	uint32_t max_cts_lost_count = 0; // continuous packet loss
	uint32_t avg_cts_lost_count = 0; // continuous packet loss
};

//==============================================================================
// 
//==============================================================================
class LostPktTracer
{
public:
	LostPktTracer(uint64_t duration);

	void Update();

	void AddLostPkt(uint32_t sn);
	void AddLostPkt(uint32_t begin_sn, uint32_t end_sn);

	void RemoveLostPkt(uint32_t sn);

	PktLostInfo GetInfo();

private:
	void ProcDuration();
	void AddLostPkt(uint32_t sn, uint64_t ts);

private:
	uint64_t m_duration = 0;
	uint64_t m_oldest_ts = 0;
	std::map<uint32_t, uint64_t> m_lost_pkts;
	std::mutex m_mutex;
};
typedef std::unique_ptr<LostPktTracer> LostPktTracerUP;

}