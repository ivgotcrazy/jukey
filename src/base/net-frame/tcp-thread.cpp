#include "tcp-thread.h"
#include "if-tcp-mgr.h"
#include "common/util-common.h"
#include "log.h"

#ifdef _WIN32
#include <WinSock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

using namespace jukey::net;


namespace
{
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SocketCallback(Socket sock, short ev, void* arg)
{
	TcpThread* obj = static_cast<TcpThread*>(arg);

	if (ev & EV_WRITE) {
		LOG_INF("EV_WRITE");
	}
	else if (ev & EV_READ) {
		obj->OnReadData(sock);
	}
	else if (ev & EV_CLOSED) {
		LOG_INF("EV_CLOSED");
	}
	else {
		LOG_INF("Unknown event {}", ev);
	}	
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ConnectCallback(Socket sock, short ev, void* arg)
{
	TcpThread* obj = static_cast<TcpThread*>(arg);
	obj->OnConnect(sock, ev);
}

}

namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
TcpThread::TcpThread(ITcpHandler* handler) : m_tcp_handler(handler)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TcpThread::OnReadData(SocketId sock)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_conns.find(sock);
	if (iter == m_conns.end()) {
		LOG_ERR("Cannot find socket {}", sock);
		return;
	}

	com::Buffer buf(4096); // TODO: 4096
	int res = recv(sock, (char*)buf.data.get(), buf.total_len, 0);
	if (res <= 0) {
		LOG_ERR("[socket:{}] recv error:{}, maybe remote:{} closed connection!", 
			sock, util::GetError(), iter->second.rep.ToStr());

		m_tcp_handler->OnConnClosed(iter->second.lep, iter->second.rep, sock);

		event_del(iter->second.ev);

		m_conns.erase(iter);
	}
	else {
		LOG_DBG("[socket:{}] Received tcp data, len:{}", sock, res);

		buf.data_len = res;
		m_tcp_handler->OnRecvTcpData(iter->second.lep, iter->second.rep, sock, buf);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TcpThread::OnConnect(SocketId sock, short ev)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_conns.find(sock);
	if (iter == m_conns.end()) {
		LOG_ERR("Cannot find socket!");
		return;
	}

	if (ev == EV_WRITE) {
		int err;
		socklen_t len = sizeof(err);
		getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
		if (err) {
			LOG_ERR("connect return ev_write, but check failed");
#ifdef _WINDOWS
      closesocket(sock);
#else
      close(sock);
#endif
			m_tcp_handler->OnConnectResult(iter->second.lep, iter->second.rep, sock, false);
		}
		else {
			event_set(iter->second.ev, sock, EV_READ | EV_PERSIST, SocketCallback, this);
			event_base_set(GetEventBase(), iter->second.ev);
			event_add(iter->second.ev, NULL);
			
			LOG_INF("Connect to {} success", iter->second.rep.ToStr());
			m_tcp_handler->OnConnectResult(iter->second.lep, iter->second.rep, sock, true);
		}
	}
	else {  // timeout
#ifdef _WINDOWS
	    closesocket(sock);
#else
        close(sock);
#endif
		LOG_INF("Connect to {} timeout", iter->second.rep.ToStr());
		m_tcp_handler->OnConnectResult(iter->second.lep, iter->second.rep, sock, false);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TcpThread::AddConnection(const com::Endpoint& lep, const com::Endpoint& rep, 
	SocketId sock)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	
	evutil_make_socket_nonblocking(sock);

	event* ev = event_new(GetEventBase(), sock,
		EV_READ | EV_CLOSED | EV_PERSIST, SocketCallback, (void*)this);
	if (!ev) {
		LOG_ERR("event_new failed!");
		return;
	}

	if (event_add(ev, NULL) < 0) {
		LOG_ERR("Add event failed!");
		return;
	}

	m_conns.insert(std::make_pair(sock, Connection(lep, rep, sock, ev)));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool TcpThread::SendData(SocketId sock, com::Buffer buf)
{
	LOG_DBG("[socket:{}] Send data, len:{}", sock, buf.data_len);

	int result = ::send(sock, (char*)(buf.data.get() + buf.start_pos), 
    (int)buf.data_len, 0);
	if (result <= 0) {
		LOG_ERR("[socket:{}] Send data failed, len:{}, error:{}", sock, 
			buf.data_len, util::GetError());
#ifdef _WINDOWS
		if (util::GetError() == WSAECONNRESET) {
			std::lock_guard<std::mutex> lock(m_mutex);
			auto iter = m_conns.find(sock);
			if (iter != m_conns.end()) {
				m_tcp_handler->OnConnClosed(iter->second.lep, iter->second.rep, sock);
				event_del(iter->second.ev);
				m_conns.erase(iter);
			}
			else {
				LOG_WRN("[socket:{}] Cannot find connection", sock);
			}
		}
#endif
		return false;
	}
	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool TcpThread::CloseConn(SocketId sock)
{
	LOG_INF("[socket:{}] Close connection", sock);

	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_conns.find(sock);
	if (iter == m_conns.end()) {
		LOG_ERR("Cannot find socket {}", sock);
		return false;
	}

	if (event_del(iter->second.ev) < 0) {
		LOG_ERR("Delete event failed, socket = {}", sock);
	}

#ifdef _WINDOWS
	if (::closesocket(sock) < 0) {
		LOG_ERR("Close socket {} failed!", sock);
	}
#else
    if (close(sock) < 0) {
		LOG_ERR("Close socket {} failed!", sock);
    }
#endif

	m_conns.erase(iter);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TcpThread::OnTcpConnectSuccess(const com::Endpoint& lep, 
	const com::Endpoint& rep, SocketId sock)
{
	event* ev = event_new(GetEventBase(), sock, EV_READ | EV_PERSIST, 
		SocketCallback, this);
	if (!ev) {
		LOG_ERR("Create event failed!");
#ifdef _WINDOWS		
        closesocket(sock);
#else
        close(sock);
#endif
		return;
	}
	event_add(ev, nullptr);

	m_conns.insert(std::make_pair(sock, Connection(lep, rep, sock, ev)));

  LOG_INF("Connect to:{} success", rep.ToStr());

	m_tcp_handler->OnConnectResult(lep, rep, sock, true);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TcpThread::OnTcpConnectFailed(const com::Endpoint& lep, 
	const com::Endpoint& rep, SocketId sock)
{
	uint64_t error = util::GetError();
#ifdef _WINDOWS
	if (error == WSAEWOULDBLOCK || error == WSAEINPROGRESS) {
#else
	if (error == EWOULDBLOCK || error == EINPROGRESS) {
#endif
		event* ev = event_new(GetEventBase(), sock, EV_WRITE, ConnectCallback, 
			this);
		if (ev) {
			struct timeval tv;
			tv.tv_sec = 5;
			tv.tv_usec = 0;
			event_add(ev, &tv);

			m_conns.insert(std::make_pair(sock, 
				Connection(lep, rep, sock, ev)));
		}
		else {
			LOG_ERR("Create event failed!");
		}
	}
	else { // Failed
		LOG_ERR("Connect to {} failed, error:{}", rep.ToStr(), error);
		m_tcp_handler->OnConnectResult(lep, rep, sock, false);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TcpThread::OnTcpConnect(TcpConnectMsgSP msg)
{
	evutil_make_socket_nonblocking(msg->sock);

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(msg->ep.port);
#ifdef _WINDOWS
	server_addr.sin_addr.S_un.S_addr = inet_addr(msg->ep.host.c_str());
#else
	server_addr.sin_addr.s_addr = inet_addr(msg->ep.host.c_str());
#endif

	int res = connect(msg->sock, (sockaddr*)&server_addr, sizeof(server_addr));
	if (res != -1) { // Success
		OnTcpConnectSuccess(GetAddress(msg->sock), msg->ep, msg->sock);
	}
	else {
		OnTcpConnectFailed(com::Endpoint(), msg->ep, msg->sock);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TcpThread::OnThreadMsg(const com::CommonMsg& msg)
{
	switch (msg.msg_type) {
	case NET_INNER_MSG_TCP_CONNECT:
		OnTcpConnect(SPC<TcpConnectMsg>(msg.msg_data));
		break;
	default:
		LOG_ERR("Unexpected msg type {}", msg.msg_type);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool TcpThread::Init()
{
	return StartThread();
}

}
