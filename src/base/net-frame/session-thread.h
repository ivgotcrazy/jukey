#pragma once

#include <list>
#include <mutex>

#include "thread/concurrent-thread.h"
#include "if-session-mgr.h"
#include "if-session.h"
#include "if-timer-mgr.h"
#include "if-sending-queue.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class SessionThread 
	: public util::ConcurrentThread
	, public std::enable_shared_from_this<SessionThread>
{
public:
	SessionThread(base::IComFactory* factory, uint32_t index);
	~SessionThread();

	void Start();
	void Stop();

private:
	void OnRecvSessionData(const com::CommonMsg& msg);
	void OnAddSession(const com::CommonMsg& msg);
	void OnRemoveSession(const com::CommonMsg& msg);
	void OnSendSessionData(const com::CommonMsg& msg);
	void OnAllocSessionId(const com::CommonMsg& msg);

	// ConcurrentThread
	virtual void OnThreadMsg(const com::CommonMsg& msg) override;
	virtual void ThreadProc() override;

	// Sending thread
	void SendingProc();

private:
	struct SessionEntry
	{
		SessionEntry(ISessionSP s, bool in) : session(s), in_sending_que(in) {}

		ISessionSP session;
		bool in_sending_que = false;
	};

private:
	std::unordered_map<SessionId, SessionEntry> m_sessions;

	ISendQueueUP m_sending_que;

	uint32_t m_thread_index = 0;

	base::IComFactory* m_factory = nullptr;

	std::thread* m_sending_thread = nullptr;
};
typedef std::shared_ptr<SessionThread> SessionThreadSP;

}