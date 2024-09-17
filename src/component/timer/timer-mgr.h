#pragma once

#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <map>

#include "thread/common-thread.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "include/if-timer-mgr.h"

namespace jukey::com
{

//==============================================================================
// TODO: This a simple timer implementation, has performance problem when
// allocating a large mount of timer
//==============================================================================
class TimerMgr 
	: public ITimerMgr
	, public base::ProxyUnknown
	, public base::ComObjTracer
	, public util::CommonThread
{
public:
	TimerMgr(base::IComFactory* factory, const char* owner);
	~TimerMgr();

	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// ITimerMgr
	virtual void Start() override;
	virtual void Stop() override;
	virtual TimerId AllocTimer(const TimerParam& param) override;
	virtual void StartTimer(TimerId timer_id) override;
	virtual void StopTimer(TimerId timer_id) override;
	virtual void FreeTimer(TimerId timer_id) override;
	virtual bool UpdateTimeout(TimerId timer_id, uint32_t timeout_ms) override;

	// CommonThread
	virtual void ThreadProc() override;

private:
	struct TimerEntry
	{
		TimerParam param;
		TimerId timer_id = INVALID_TIMER_ID;
	};
	typedef std::shared_ptr<TimerEntry> TimerEntrySP;

	struct TimerNode
	{
		uint64_t timeout_stamp = 0;
		std::list<TimerEntrySP> entry_list;
	};

private:
	typedef std::shared_ptr<TimerNode> TimerNodeSP;
	typedef std::list<TimerNodeSP> TimerNodeList;
	typedef TimerNodeList::iterator TimerNodeListIter;

	static const uint32_t TIMER_BASE_TIME_MS    = 1;
	static const uint32_t TIMER_NODE_MAX_COUNT  = 32768; // 1024 * 32
	static const uint32_t TIMER_ENTRY_MAX_COUNT = 1024;

private:
	void TimerProc();
	void AddEntryWithTimeout(uint32_t timeout, TimerEntrySP entry);
	void AddEntryAsNewNode(TimerNodeListIter iter, uint64_t ts, TimerEntrySP entry);
	void DoStopTimer(TimerId timer_id);
	uint32_t GetWorkingTimerSize();

private:
	TimerNodeList m_node_list;

	std::map<TimerId, TimerParam> m_idle_timers;

	// Max allocated timer ID
	TimerId m_alloc_timer_id = 0;

	// timers that did not start, or timeout timers
	//std::set<TimerId> m_idle_timers;

	// 定时器回调函数中再调用 TimerMgr 方法，比如定时器超时后立马启动下一个定时器
	std::recursive_mutex m_mutex;
};

} // namespace