#include "event-thread.h"
#include "log/util-log.h"
#include "common-config.h"
#include "event.h"

#ifdef _WIN32
#include <WinSock2.h>
#endif

#ifdef _LINUX
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

namespace
{

using namespace jukey::util;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void PostCallback(Socket sock, short ev, void* arg)
{
	EventThread* obj = static_cast<EventThread*>(arg);
	obj->OnPostMsg(sock);
}

}

namespace jukey::util
{
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
EventThread::EventThread() 
	: m_msg_queue("EventThread")
	, m_post_port(EVENT_THREAD_BIND_INIT_PORT)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
EventThread::~EventThread()
{
	StopThread();

	event_base_free(m_ev_base);
	m_ev_base = nullptr;

#ifdef _WINDOWS
	closesocket(m_send_sock);
	closesocket(m_recv_sock);
#elif _LINUX
	close(m_send_sock);
	close(m_recv_sock);
#endif


}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void EventThread::PostMsg(const com::CommonMsg& msg)
{
	m_msg_queue.PushMsg(msg);

	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");
	sin.sin_port = htons(m_post_port);

	int res = sendto(m_send_sock, "post", (int)strlen("post"), 0,
		(sockaddr*)& sin, (int)sizeof(sin));
	if (res <= 0) {
		UTIL_ERR("Send data to post sock failed!");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void EventThread::OnPostMsg(Socket sock)
{
	char buf[64];
	struct sockaddr_in sin;
	int len = sizeof(sin);
#ifdef _WINDOWS
	if (recvfrom(sock, buf, 64, 0, (sockaddr*)&sin, &len) <= 0) {
		UTIL_ERR("recvfrom failed, error = {}", ::GetLastError());
	}
#else
	if (recvfrom(sock, buf, 64, 0, (sockaddr*)&sin, (socklen_t*)&len) <= 0) {
		UTIL_ERR("recvfrom failed, error = {}", errno);
	}
#endif

	while (!m_msg_queue.IsEmpty()) {
		com::CommonMsg msg;
		if (m_msg_queue.PopMsg(msg)) {
			OnThreadMsg(msg);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool EventThread::StartThread()
{
	event* ev = nullptr;

	m_ev_base = event_base_new();
	if (!m_ev_base) {
		UTIL_ERR("Create event base failed!");
		return false;
	}

	m_send_sock = ::socket(PF_INET, SOCK_DGRAM, 0);
	if (m_send_sock == -1) {
		UTIL_ERR("Create post send socket failed!");
		goto EXIT;
	}

	m_recv_sock = ::socket(PF_INET, SOCK_DGRAM, 0);
	if (m_send_sock == -1) {
		UTIL_ERR("Create post recv socket failed!");
		goto EXIT;
	}

	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");

	for (int i = 0; i < EVENT_THREAD_MAX_BIND_TRY_COUNT; i++) {
		sin.sin_port = htons(m_post_port);
		if (::bind(m_recv_sock, (sockaddr*)& sin, sizeof(sin)) < 0) {
			if (i == EVENT_THREAD_MAX_BIND_TRY_COUNT - 1) {
				UTIL_ERR("Bind socket failed!");
				goto EXIT;
			}
		}
		else {
			UTIL_INF("Bind recv socket on port {} success", m_post_port);
			break;
		}
		++m_post_port;
	}

	ev = event_new(m_ev_base, m_recv_sock, EV_READ|EV_PERSIST, PostCallback, this);
	if (!ev) {
		UTIL_ERR("Create event failed!");
		goto EXIT;
	}
	event_add(ev, nullptr);

	m_thread = std::thread(&EventThread::ThreadProc, this);

	return true;

EXIT:
	if (m_recv_sock != -1) {
		evutil_closesocket(m_recv_sock);
	}

	if (m_send_sock != -1) {
		evutil_closesocket(m_send_sock);
	}

	return false;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void EventThread::StopThread()
{
	event_base_loopbreak(m_ev_base);

	if (m_thread.joinable()) {
		m_thread.join();
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void EventThread::ThreadProc()
{
	UTIL_INF("Enter event thread");

	event_base_loop(m_ev_base, EVLOOP_NO_EXIT_ON_EMPTY);

	UTIL_INF("Exit event thread");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void EventThread::OnThreadMsg(const com::CommonMsg& msg)
{
	UTIL_INF("OnThreadMsg, msg:{}", msg.msg_type);
}

}
