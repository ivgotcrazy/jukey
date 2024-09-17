#pragma once

#include <map>
#include <memory>
#include <mutex>

#include "common-struct.h"
#include "defer.h"
#include "thread/if-thread.h"
#include "com-factory.h"
#include "if-timer-mgr.h"


namespace jukey::util
{

//==============================================================================
// 
//==============================================================================
class AsyncProxyBase : public std::enable_shared_from_this<AsyncProxyBase>
{
public:
	AsyncProxyBase(base::IComFactory* factory, util::IThread* thread, 
		uint32_t timeout);
	~AsyncProxyBase();

	void Stop();

	void OnTimer(int64_t param);

protected:
	void StartAsyncTimer(uint32_t seq, uint32_t msg, uint32_t usr, DeferSP defer);
	DeferSP GetDefer(uint32_t seq, uint32_t msg, uint32_t usr);
	void MakeAsyncError(const std::string& msg, DeferSP defer);
	bool SaveDefer(uint32_t seq, uint32_t msg, uint32_t usr, DeferSP defer);

private:
	struct AsyncEntry
	{
		AsyncEntry(uint32_t s, uint32_t m, uint32_t u) 
			: seq(s), msg(m), usr(u) {}

		uint32_t seq = 0;
		uint32_t msg = 0;
		uint32_t usr = 0;
	};

	struct AsyncKey
	{
		AsyncKey(uint32_t s, uint32_t m) : seq(s), msg(m) {}

		bool operator<(const AsyncKey& key) const
		{
			if (seq < key.seq) {
				return true;
			}
			else if (seq == key.seq) {
				return msg < key.msg;
			}
			else {
				return false;
			}
		}

		uint32_t seq = 0;
		uint32_t msg = 0;
	};

	// If user is 0, it means matching any user.
	// Therefore, if there exists a key with user 0, there cannot exist keys with
	//  non-zero user.
	// Conversely, if there exists a key with non-zero user, there cannot exist a
	//  key with user 0.
	// However, there can be multiple keys with non-zero user to support matching
	// for different users.
	
	// user:Defer | err_defer_seq:Defer
	typedef std::map<uint32_t, DeferSP> DeferMap;

	// msg+seq:DeferMap
	typedef std::map<AsyncKey, DeferMap> AsyncMap;

private:
	base::IComFactory* m_factory = nullptr;
	com::ITimerMgr* m_timer_mgr = nullptr;
	util::IThread* m_thread = nullptr;
	
	uint32_t m_timeout = 0;
	
	AsyncMap m_async_calls;
	std::mutex m_mutex;

	DeferMap m_err_defers; // err_defer_seq:defer
	uint32_t m_err_defer_seq = 0;

	bool m_stop = false;
};
typedef std::shared_ptr<AsyncProxyBase> AsyncProxyBaseSP;

}