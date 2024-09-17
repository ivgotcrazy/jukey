#pragma once

#include <map>
#include <set>
#include <memory>

#include "common-struct.h"
#include "if-stream-receiver.h"
#include "com-factory.h"
#include "if-timer-mgr.h"

namespace jukey::txp
{

//==============================================================================
// 
//==============================================================================
class INackRequestHandler
{
public:
	virtual void OnNackRequest(uint32_t count, const com::Buffer& buf) = 0;
};

//==============================================================================
// 
//==============================================================================
class NackRequester
{
public:
	NackRequester(base::IComFactory* factory, INackRequestHandler* handler);
	~NackRequester();

	void OnRecvPacket(const com::Buffer& buf);
	void UpdateRTT(uint32_t rtt_ms);
	void OnPreRecvPacket(uint32_t seq);

	void OnTimer();

private:
	uint32_t GetFirstReqIntervalMs();

private:
	base::IComFactory* m_factory = nullptr;
	INackRequestHandler* m_handler = nullptr;

	com::ITimerMgr* m_timer_mgr = nullptr;
	com::TimerId m_timer_id = INVALID_TIMER_ID;

	struct NackItem
	{
		uint64_t create_us;
		uint32_t seq;
		uint64_t sent_us;
		uint32_t retries;
	};

	// key:seq
	std::map<uint32_t, NackItem> m_nack_map;

	static const uint32_t kNackTimeoutMs = 1000;
	static const uint32_t kNackMaxRetries = 16;
	static const uint32_t kNackFirstReqRttMultiple = 3;
	static const uint32_t kNackIntervalRttMultiple = 2;

	uint32_t m_rtt_ms = 20; // 默认 20ms

	int64_t m_max_recv_seq = -1;
	uint32_t m_max_recv_gseq = 0;

	std::mutex m_mutex;

	std::set<uint32_t> m_pre_recv_pkts;
};
typedef std::unique_ptr<NackRequester> NackRequesterUP;

}