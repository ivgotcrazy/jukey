#include "session-thread.h"
#include "net-inner-message.h"
#include "common-config.h"
#include "if-timer-mgr.h"
#include "common/util-time.h"
#include "event/common-event.h"
#include "sending-queue.h"
#include "log.h"

using namespace jukey::util;

namespace
{
using namespace jukey::net;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string GetSessionThreadName(uint32_t index)
{
	return std::string("SessionThread_").append(std::to_string(index));
}

}

namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionThread::SessionThread(base::IComFactory* factory, uint32_t index)
	: ConcurrentThread(GetSessionThreadName(index), SESSION_THREAD_MSG_QUEUE_SIZE)
	, m_thread_index(index)
	, m_factory(factory)
{
  m_sending_que.reset(new SendingQueue());

  m_sending_thread = new std::thread(&SessionThread::SendingProc, this);
  assert(m_sending_thread);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionThread::~SessionThread()
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionThread::Start()
{
	StartThread();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionThread::Stop()
{
  if (m_sending_thread) {
    delete m_sending_thread;
    m_sending_thread = nullptr;
  }

	StopThread();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionThread::OnRecvSessionData(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(ConnectionDataMsg);

	LOG_DBG("[session:{}] Received data, len:{}", data->sid, data->buf.data_len);

	auto iter = m_sessions.find(data->sid);
	if (iter != m_sessions.end()) {
		iter->second.session->OnRecvData(data->buf);
	}
	else {
		LOG_ERR("Cannot find session {}", data->sid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionThread::OnAddSession(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(ISession);

	if (m_sessions.find(data->GetParam().local_sid) == m_sessions.end()) {
		data->Init(); // Initialize in session thread

    SessionEntry entry(data, false);
		m_sessions.insert(std::make_pair(data->GetParam().local_sid, entry));

		LOG_INF("Session thread {} add session {}, total session count:{}",
			m_thread_index, data->GetParam().local_sid, m_sessions.size());
	}
	else {
		LOG_ERR("Session {} already exists!", data->GetParam().local_sid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionThread::OnRemoveSession(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(RemoveSessionMsg);
	
	auto iter = m_sessions.find(data->sid);
	if (iter != m_sessions.end()) {
		iter->second.session->Close(data->active);
		m_sessions.erase(iter);
		m_sending_que->RemoveEntry(data->sid);
		LOG_INF("Session thread {} remove session {}, total session count:{}",
			m_thread_index, data->sid, m_sessions.size());
	}
	else {
		LOG_WRN("Cannot find session {} to remove!", data->sid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionThread::OnSendSessionData(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(SendSessionDataMsg);

	auto iter = m_sessions.find(data->sid);
	if (iter != m_sessions.end()) {
		iter->second.session->OnSendData(data->buf);
		// ugly!!!
		m_sending_que->UpdateEntry(
			std::dynamic_pointer_cast<ISendEntry>(iter->second.session), false);
	}
	else {
		LOG_DBG("Cannot find session {} to send data", data->sid);
		data->result.set_value(false);
	}
}

//------------------------------------------------------------------------------
// Request available session ID based on the total number of session threads 
// and the current session thread index
//------------------------------------------------------------------------------
void SessionThread::OnAllocSessionId(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(FetchSessionIdMsg);

	for (int i = 0; i < 0xFFFF; i++) {
		uint32_t sid = data->thread_count * i + data->thread_index;
		
    // No avaliable session ID
		if (sid > 0xFFFF) break;

    // Invalid session ID
		if (sid == 0) continue;

    // In use
		if (m_sessions.find((uint16_t)sid) == m_sessions.end()) {
			continue;
		}
		
    // Found one 
		data->sid.set_value(sid);
		return;
	}

  // Failed
	data->sid.set_value(INVALID_SESSION_ID);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionThread::OnThreadMsg(const com::CommonMsg& msg)
{
	switch (msg.msg_type) {
	case NET_INNER_MSG_RECV_SESSION_DATA:
		OnRecvSessionData(msg);
		break;
	case NET_INNER_MSG_ADD_SESSION:
		OnAddSession(msg);
		break;
	case NET_INNER_MSG_REMOVE_SESSION:
		OnRemoveSession(msg);
		break;
	case NET_INNER_MSG_SEND_SESSION_DATA:
		OnSendSessionData(msg);
		break;
	case NET_INNER_MSG_ALLOC_SESSION_ID:
		OnAllocSessionId(msg);
		break;
	default:
		LOG_ERR("Unexpected message {}", msg.msg_type);
	}
}

//------------------------------------------------------------------------------
// Sending thread
//------------------------------------------------------------------------------
void SessionThread::SendingProc()
{
  while (!m_stop) {
    ISendEntrySP send_entry = m_sending_que->GetNextEntry();
    if (send_entry) {
      // Do send data
      send_entry->SendData();
      // Has more data to be sent
      if (send_entry->NextSendTime() != INVALID_SEND_TIME) {
        m_sending_que->AddEntry(send_entry);
      }
    }
  }
}

//------------------------------------------------------------------------------
// Message thread
//------------------------------------------------------------------------------
void SessionThread::ThreadProc()
{
	LOG_INF("Enter session thread:{}", m_thread_index);

  com::CommonMsg msg;
  uint64_t next_update = 0, now = 0, wait_time = 0;

  while (!m_stop) {
    now = util::Now();

    // Update loop
    if (next_update <= now) {
      for (auto iter : m_sessions) {
        iter.second.session->OnUpdate();
      }
      next_update = now + SESSION_UPDATE_INTERVAL;
    }

    wait_time = next_update > now ? next_update - now : 0;

    if (m_msg_queue.wait_dequeue_timed(msg, wait_time)) {
      if (msg.msg_type == QUIT_THREAD_MSG) {
        LOG_INF("Quit thread!");
        break;
      }
      else {
        OnThreadMsg(msg);
      }
    }
  }

	LOG_INF("Exit session thread:{}", m_thread_index);
}

}