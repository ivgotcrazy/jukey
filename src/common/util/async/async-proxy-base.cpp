#include "async-proxy-base.h"
#include "common-error.h"
#include "common-message.h"
#include "common-define.h"
#include "protocol.h"


using namespace jukey::util;
using namespace jukey::com;

using namespace std::placeholders;

namespace jukey::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AsyncProxyBase::AsyncProxyBase(base::IComFactory* factory, util::IThread* thread,
	uint32_t timeout) : m_factory(factory), m_thread(thread), m_timeout(timeout)
{
	m_timer_mgr = QUERY_TIMER_MGR(m_factory);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AsyncProxyBase::~AsyncProxyBase()
{
	UTIL_INF("{}", __FUNCTION__);

	Stop();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AsyncProxyBase::Stop()
{
	m_stop = true;

	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto& item : m_async_calls) {
		for (auto& entry : item.second) {
			m_timer_mgr->StopTimer(entry.second->TimerId());
			m_timer_mgr->FreeTimer(entry.second->TimerId());
		}
	}

	m_async_calls.clear();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AsyncProxyBase::MakeAsyncError(const std::string& msg, DeferSP defer)
{
	if (m_stop) return;

	uint32_t seq = ++m_err_defer_seq;
	m_err_defers.insert(std::make_pair(seq, defer));

	m_thread->Execute([this, seq](CallParam) -> void {
		DeferSP defer;
		{
			std::lock_guard<std::mutex> lock(m_mutex);

			auto iter = m_err_defers.find(seq);
			if (iter != m_err_defers.end()) {
				defer = iter->second;
				m_err_defers.erase(iter);
			}
			else {
				UTIL_ERR("cannot find err defer by seq, seq:{}", seq);
				return;
			}
		}
		defer->ReportError("failed");
	}, shared_from_this());
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool AsyncProxyBase::SaveDefer(uint32_t seq, uint32_t msg, uint32_t usr, 
	DeferSP defer)
{
	UTIL_INF("Save defer, seq:{}, msg:{}, usr:{}", seq, msg, usr);

	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_async_calls.find(AsyncKey(seq, msg));
	if (iter != m_async_calls.end()) {
		UTIL_DBG("Find defer map");

		if (iter->second.empty()) {
			UTIL_DBG("Defer map is empty, insert defer");
			iter->second.insert(std::make_pair(usr, defer));
		}
		else {
			if (usr == 0) {
				MakeAsyncError("user is 0, but has defer already", defer);
				UTIL_ERR("user is 0, but has defer already");
				return false;
			}

			if (iter->second.find(0) != iter->second.end()) {
				MakeAsyncError("incomming user is not 0, but find 0 user", defer);
				UTIL_ERR("incomming user is not 0, but find 0 user");
				return false;
			}

			if (iter->second.find(usr) != iter->second.end()) {
				MakeAsyncError("user is already exists", defer);
				UTIL_ERR("user is already exists");
				return false;
			}

			UTIL_INF("Defer map is not empty, insert defer");
			iter->second.insert(std::make_pair(usr, defer));
		}
	}
	else {
		UTIL_DBG("No defer map, create defer map and insert defer");

		DeferMap defer_map;
		defer_map.insert(std::make_pair(usr, defer));
		m_async_calls.insert(std::make_pair(AsyncKey(seq, msg), defer_map));
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AsyncProxyBase::StartAsyncTimer(uint32_t seq, uint32_t msg, uint32_t usr,
	DeferSP defer)
{
	com::TimerParam timer_param;
	timer_param.timer_type = TimerType::TIMER_TYPE_ONCE;
	timer_param.timeout    = m_timeout;
	timer_param.timer_name = "mq aysnc proxy timer";
	timer_param.user_data  = (int64_t)new AsyncEntry(seq, msg, usr);
	timer_param.timer_func = std::bind(&AsyncProxyBase::OnTimer, this, _1);

	TimerId timer_id = m_timer_mgr->AllocTimer(timer_param);
	m_timer_mgr->StartTimer(timer_id);

	defer->SetTimerId(timer_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
DeferSP AsyncProxyBase::GetDefer(uint32_t seq, uint32_t msg, uint32_t usr)
{
	UTIL_INF("Get defer, seq:{}, msg:{}, usr:{}", seq, msg, usr);

	std::lock_guard<std::mutex> lock(m_mutex);

	DeferSP defer;
	do {
		// Find seq+msg entry(map)
		auto iter = m_async_calls.find(AsyncKey(seq, msg));
		if (iter == m_async_calls.end()) {
			UTIL_ERR("Cannot find defer map, seq:{}, msg:{}", seq, msg);
			break;
		}

		UTIL_DBG("Find defer map");

		// Find by exact match
		auto user_iter = iter->second.find(usr);
		if (user_iter == iter->second.end()) {
			UTIL_DBG("Cannot find exact user in defer map");

			// Find user 0 if excat match failed
			user_iter = iter->second.find(0);
			if (user_iter == iter->second.end()) {
				UTIL_ERR("Cannot find user 0 entry");
				break;
			}
		}

		defer = user_iter->second;

		UTIL_DBG("Find defer entry, remove it from defer map");

		// Remove from user map
		iter->second.erase(user_iter);

		// Remov from async map if user map is empty
		if (iter->second.empty()) {
			UTIL_DBG("Defer map is empty, remove defer map");
			m_async_calls.erase(iter);
		}
	} while (false);

	if (defer) {
		m_timer_mgr->StopTimer(defer->TimerId());
		m_timer_mgr->FreeTimer(defer->TimerId());
	}

	return defer;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AsyncProxyBase::OnTimer(int64_t param)
{
	if (m_stop) return;

	m_thread->Execute([this, param](CallParam) -> void {
		AsyncEntry* entry = (AsyncEntry*)param;
		DeferSP defer = GetDefer(entry->seq, entry->msg, entry->usr);
		if (defer) {
			defer->ReportTimeout();
		}
		delete entry;
	}, shared_from_this());
}

}