#include "timer-mgr.h"
#include "log.h"
#include "common-struct.h"
#include "common/util-time.h"
#include "common/execution-timer.h"

namespace jukey::com
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
TimerMgr::TimerMgr(base::IComFactory* factory, const char* owner)
	: base::ProxyUnknown(nullptr)
	, base::ComObjTracer(factory, CID_TIMER_MGR, owner)
	, util::CommonThread("timer-mgr", true)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
TimerMgr::~TimerMgr()
{
	LOG_DBG("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* TimerMgr::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_TIMER_MGR) == 0) {
		return new TimerMgr(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* TimerMgr::NDQueryInterface(const char* riid)
{
	if (0 == strcmp(riid, IID_TIMER_MGR)) {
		return static_cast<ITimerMgr*>(this);
	}
	else {
		return ProxyUnknown::NDQueryInterface(riid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TimerMgr::Start()
{
	StartThread();

  LOG_INF("Start timer manager");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TimerMgr::Stop()
{
	StopThread();

	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	m_idle_timers.clear();
	m_node_list.clear();

	LOG_INF("Stop timer manager");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
TimerId TimerMgr::AllocTimer(const TimerParam& param)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	if (m_node_list.size() >= TIMER_NODE_MAX_COUNT) {
		LOG_WRN("Reach max node size!");
		return 0;
	}

	TimerId timer_id = ++m_alloc_timer_id;

	m_idle_timers.insert(std::make_pair(timer_id, param));

	LOG_DBG("Alloc timer {}", timer_id);

	return timer_id;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TimerMgr::StartTimer(TimerId timer_id)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	auto iter = m_idle_timers.find(timer_id);
	if (iter == m_idle_timers.end()) {
		LOG_ERR("Cannot find timer:{} in idle list!", timer_id);
		return;
	}

	LOG_DBG("Start timer, id:{}, name:{}, timout:{}", timer_id, 
		iter->second.timer_name, iter->second.timeout);

	TimerEntrySP entry(new TimerEntry());
	entry->param = iter->second;
	entry->timer_id = timer_id;

	// Add to timeout list
	if (entry->param.run_atonce) {
		AddEntryWithTimeout(0, entry);
	}
	else {
		AddEntryWithTimeout(entry->param.timeout * 1000, entry);
	}

	// Remove from the idle list
	m_idle_timers.erase(iter);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TimerMgr::AddEntryWithTimeout(uint32_t timeout, TimerEntrySP entry)
{
	LOG_DBG("Add timer entry, id:{}, name:{}, type:{}, timeout:{}", 
		entry->timer_id, 
		entry->param.timer_name, 
		entry->param.timer_type, 
		timeout);

	uint64_t stamp = util::Now() + timeout;
  
	LOG_DBG("Before timer node list size:{}", m_node_list.size());

	// Traverse all nodes, looking for insertion point
	auto iter = m_node_list.begin();
	for (; iter != m_node_list.end(); iter++) {
		// Find the node with the same timeout value
		// TODO: 
		if (stamp == (*iter)->timeout_stamp) {
			(*iter)->entry_list.push_back(entry);
			LOG_DBG("Add timer entry to exist node!");
			break;
		}
		// Insert before the node who is the first node that has a bigger timeout_stamp
		if (stamp < (*iter)->timeout_stamp) {
			AddEntryAsNewNode(iter, stamp, entry);
			break;
		}
	}

	// After looking around, couldn't find any insertion point, so added it 
	// directly to the end
	if (iter == m_node_list.end()) {
		AddEntryAsNewNode(iter, stamp, entry);
	}

	LOG_DBG("After timer node list size:{}", m_node_list.size());
}
  
//------------------------------------------------------------------------------
// 之所以这里要用异步处理，是因为可能出现在 OnTimer 中调用 StopTimer 的操作
//------------------------------------------------------------------------------
void TimerMgr::StopTimer(TimerId timer_id)
{
	LOG_DBG("Stop timer:{}", timer_id);

	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	DoStopTimer(timer_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TimerMgr::FreeTimer(TimerId timer_id)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	if (m_idle_timers.find(timer_id) != m_idle_timers.end()) {
		m_idle_timers.erase(timer_id); // no record fail here
		LOG_DBG("Free timer:{} success", timer_id);
	}
	else {
		LOG_WRN("Cannot find timer:{} to free in idle list!", timer_id);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool TimerMgr::UpdateTimeout(TimerId timer_id, uint32_t timeout_ms)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	auto iter = m_idle_timers.find(timer_id);
	if (iter == m_idle_timers.end()) {
		LOG_DBG("Cannot find timer to update timeout, id:{}", timer_id);
		return false;
	}

	iter->second.timeout = timeout_ms;

	return true;
}

//------------------------------------------------------------------------------
// Note: performance!!!
//------------------------------------------------------------------------------
void TimerMgr::DoStopTimer(TimerId timer_id)
{
	for (auto i = m_node_list.begin(); i != m_node_list.end(); i++) {
		for (auto j = (*i)->entry_list.begin(); j != (*i)->entry_list.end(); j++) {
			if ((*j)->timer_id == timer_id) {
				LOG_DBG("Stop timer:{} success", timer_id);
				m_idle_timers.insert(std::make_pair(timer_id, (*j)->param));
				(*i)->entry_list.erase(j);
				return; // return directly after processed
			}
		}
	}

	if (m_idle_timers.find(timer_id) != m_idle_timers.end()) {
		LOG_DBG("Timer:{} is in idle list!", timer_id);
	} else {
		LOG_ERR("Cannot find timer:{} to stop!", timer_id);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t TimerMgr::GetWorkingTimerSize()
{
	uint32_t count = 0;
	for (auto i = m_node_list.begin(); i != m_node_list.end(); i++) {
		count += static_cast<uint32_t>((*i)->entry_list.size());
	}
	return count;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TimerMgr::TimerProc()
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	uint64_t now = util::Now();

	// Traverse all timer nodes
	for (auto node_iter = m_node_list.begin(); node_iter != m_node_list.end(); ) {
		if (now < (*node_iter)->timeout_stamp) {
			break; // needn't compare any more
		}

		// All timers under the current node have to process timeout
		for (auto entry_iter = (*node_iter)->entry_list.begin(); 
			entry_iter != (*node_iter)->entry_list.end(); ++entry_iter) {
			///---
			(*entry_iter)->param.timer_func((*entry_iter)->param.user_data);
			///---

			// Add to timer list again if it's a loop timer
			if ((*entry_iter)->param.timer_type == TimerType::TIMER_TYPE_LOOP) {
				AddEntryWithTimeout((*entry_iter)->param.timeout * 1000, (*entry_iter));
			} 
			else { // Delete it and add to idle list if it's a once timer
				LOG_DBG("Move once timer:{} to idle list.", (*entry_iter)->timer_id);
				m_idle_timers.insert(std::make_pair((*entry_iter)->timer_id, 
					(*entry_iter)->param));
			}
		}

		// Remove it after being processed
		node_iter = m_node_list.erase(node_iter);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TimerMgr::ThreadProc()
{
	LOG_INF("Start timer thread.");

  int64_t proc_duration = 0;

	while (!m_stop) {
	  int64_t sleep_duration = TIMER_BASE_TIME_MS * 1000 - proc_duration;
	  if (sleep_duration > 0) {
	    util::Sleep(sleep_duration);
	  }		

	  uint64_t begin = util::Now();
	  TimerProc();
	  uint64_t end = util::Now();

	  proc_duration = end - begin;
	}

	LOG_INF("Exit timer thread.");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TimerMgr::AddEntryAsNewNode(TimerNodeListIter iter, uint64_t ts, 
	TimerEntrySP entry)
{
	TimerNodeSP node(new TimerNode());
	node->timeout_stamp = ts;
	node->entry_list.push_back(entry);

	if (iter == m_node_list.end()) {
		m_node_list.push_back(node);
	} else {
		m_node_list.insert(iter, node);
	}
}

}
