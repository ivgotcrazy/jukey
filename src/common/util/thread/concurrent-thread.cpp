#include "concurrent-thread.h"
#include "log/util-log.h"
#include "common-config.h"


namespace jukey::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ConcurrentThread::ConcurrentThread(CSTREF owner)
	: m_owner(owner)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ConcurrentThread::ConcurrentThread(CSTREF owner, uint32_t max_que_size)
	: m_msg_queue(max_que_size)
	, m_owner(owner)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ConcurrentThread::~ConcurrentThread()
{
	StopThread();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ConcurrentThread::PostMsg(const com::CommonMsg& msg)
{
	if (!m_msg_queue.try_enqueue(msg)) {
		UTIL_DBG("Post message to {} failed!", m_owner);
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ConcurrentThread::StartThread()
{
	UTIL_INF("Start thread:{}", m_owner);

	m_stop = false;
	m_thread = std::thread(&ConcurrentThread::ThreadProc, this);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ConcurrentThread::StopThread()
{
	UTIL_INF("Stop thread:{}", m_owner);

	if (!m_stop) {
		m_stop = true;
		PostMsg(com::CommonMsg(QUIT_THREAD_MSG));

		if (m_thread.joinable()) {
			m_thread.join();
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ConcurrentThread::ThreadProc()
{
	UTIL_INF("Enter thread:{}", m_owner);

	com::CommonMsg msg;
	while (!m_stop) {
		m_msg_queue.wait_dequeue(msg);
		if (msg.msg_type == QUIT_THREAD_MSG) {
			UTIL_INF("Quit thread!");
			break;
		}
		else {
			OnThreadMsg(msg);
		}
	}

	UTIL_INF("Exit thread:{}", m_owner);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ConcurrentThread::OnThreadMsg(const com::CommonMsg& msg)
{
	UTIL_INF("OnThreadMsg, msg[{}]", msg.msg_type);
}

}