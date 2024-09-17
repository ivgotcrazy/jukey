#pragma once

#include <map>
#include <memory>
#include "include/if-pinger.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "if-timer-mgr.h"
#include "thread/common-thread.h"

namespace jukey::com
{

//==============================================================================
// 
//==============================================================================
class Pinger 
	: public IPinger
	, public base::ProxyUnknown
	, public base::ComObjTracer
	, public util::CommonThread
{
public:
	Pinger(base::IComFactory* factory, const char* owner);

	// ProxyUnknown
	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IPinger
	virtual bool Init(const ServiceParam& param, IPingHandler* handler, 
		uint32_t ping_interval) override;
	virtual void Start() override;
	virtual void Stop() override;
	virtual void AddPingService(const ServiceParam& param) override;
	virtual void OnRecvPongMsg(const com::Buffer& sig_buf) override;

	// CommonThread
	virtual void OnThreadMsg(const com::CommonMsg& msg) override;

	void SendPingMsg();
	void CheckTimeout();

private:
	void StartPingTimer();
	void StartTimeoutTimer();

private:
	struct PingEntry
	{
		ServiceParam service;
		uint64_t last_ping = 0;
		uint32_t last_seq = 0;
	};

private:
	base::IComFactory* m_factory = nullptr;

	ServiceParam m_param;
	IPingHandler* m_handler = nullptr;
	uint32_t m_ping_interval = 0;

	std::vector<PingEntry> m_ping_entries;

	com::ITimerMgr* m_timer_mgr = nullptr;
	com::TimerId m_ping_timer = INVALID_TIMER_ID;
	com::TimerId m_timeout_timer = INVALID_TIMER_ID;

	std::mutex m_mutex;
};

}