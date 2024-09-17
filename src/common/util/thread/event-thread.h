#pragma once

#include <thread>
#include <memory>

#include "common-struct.h"
#include "msg-bus/msg-queue.h"
#include "net-public.h"

struct event_base;

namespace jukey::util
{

//==============================================================================
// 
//==============================================================================
class EventThread
{
public:
	EventThread();
	virtual ~EventThread();

	bool StartThread();

	void StopThread();

	virtual void PostMsg(const com::CommonMsg& msg);

	void OnPostMsg(Socket sock);

	event_base* GetEventBase() { return m_ev_base; }

protected:
	util::MsgQueue<com::CommonMsg> m_msg_queue;

private:
	// Overwrite this method to process message directly
	virtual void OnThreadMsg(const com::CommonMsg& msg);
	
	void ThreadProc();

private:
	std::thread m_thread;

	// SocketPair
	Socket m_send_sock = -1;
	Socket m_recv_sock = -1;
	uint16_t m_post_port = 0;

	event_base* m_ev_base = nullptr;
};

typedef std::shared_ptr<EventThread> EventThreadSP;

} // namespace