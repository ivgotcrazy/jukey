#include "udp-manager.h"
#include "net-common.h"
#include "event2/thread.h"
#include "common-config.h"
#include "common/util-common.h"
#include "log.h"

#ifdef _LINUX
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

namespace
{

using namespace jukey::net;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SocketCallback(Socket sock, short ev, void* arg)
{
	UdpManager* obj = static_cast<UdpManager*>(arg);

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

}

namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
UdpManager::UdpManager(base::IComFactory* factory, const char* owner)
	: ProxyUnknown(nullptr)
  , ComObjTracer(factory, CID_UDP_MGR, owner)
	, CommonThread("UdpManager", true)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
UdpManager::~UdpManager()
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* UdpManager::CreateInstance(base::IComFactory* factory,
  const char* cid, const char* owner)
{
  if (strcmp(cid, CID_UDP_MGR) == 0) {
    return new UdpManager(factory, owner);
  }
  else {
    LOG_ERR("Invalid cid {}", cid);
    return nullptr;
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* UdpManager::NDQueryInterface(const char* riid)
{
  if (0 == strcmp(riid, IID_UDP_MGR)) {
    return static_cast<IUdpMgr*>(this);
  }
  else {
    return ProxyUnknown::NDQueryInterface(riid);
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void UdpManager::OnReadData(Socket sock)
{
	com::Buffer recv_buf(UDP_RECV_BUF_LEN);
	int size = sizeof(struct sockaddr);
	struct sockaddr_in addr;
  com::Endpoint local_ep;

  // Must exist
  m_mutex.lock();
  auto iter = m_sock_items.find(sock);
  if (iter == m_sock_items.end()) {
    LOG_ERR("Cannot find sock item:{}", sock);
    m_mutex.unlock();
    return;
  }
  else {
    local_ep = iter->second.ep;
    m_mutex.unlock();
  }

#ifdef _WINDOWS
	int res = recvfrom(
		sock, 
		(char*)recv_buf.data.get(), 
		UDP_RECV_BUF_LEN, 
		0, 
		(sockaddr*)&addr, 
		&size);
#else
	int res = recvfrom(
		sock, 
		(char*)recv_buf.data.get(), 
		UDP_RECV_BUF_LEN, 
		0, 
		(sockaddr*)&addr, 
		(socklen_t*)&size);
#endif

	// Remote endpoint
	com::Endpoint remote_ep(inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	if (res <= 0) {
		LOG_WRN("Socket {} recv data failed, error:{}", sock, util::GetError());
		
		m_udp_handler->OnSocketClosed(local_ep, remote_ep, sock);
    return;
	}
	
	// Set data length
  recv_buf.data_len = res;

  LOG_DBG("Received udp data from:{}, len = {}", remote_ep.ToStr(), res);

  // Receive callback
	m_udp_handler->OnRecvUdpData(local_ep, remote_ep, sock, recv_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode UdpManager::Init(IUdpHandler* handler)
{
	m_udp_handler = handler;

#ifdef _WINDOWS
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		LOG_ERR("WSAStartup failed!");
		return com::ErrCode::ERR_CODE_FAILED;
	}

	evthread_use_windows_threads();
#endif

	m_ev_base = event_base_new();
	if (!m_ev_base) {
		LOG_ERR("Create event base failed!");
		return com::ErrCode::ERR_CODE_FAILED;
	}

	StartThread();

	return com::ErrCode::ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Socket UdpManager::CreateSocket(const com::Endpoint* ep)
{
	Socket sock = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1) {
		LOG_ERR("Create socket failed, error:{}", util::GetError());
		return INVALID_SOCKET_ID;
	}

	LOG_INF("Create udp socket:{}, address:{}", sock, GetAddress(sock).ToStr());

	/* Set socket option */
	int flag = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag)) < 0) {
		LOG_ERR("setsockopt failed, error:{}!", util::GetError());
		return INVALID_SOCKET_ID;
	}

	int buf_size = -1;
	socklen_t optlen = sizeof(buf_size);
	if (getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&buf_size, &optlen) < 0)
	{
		LOG_ERR("getsockopt failed, error:{}", util::GetError());
		return INVALID_SOCKET_ID;
	}
	else {
		LOG_INF("Get udp socket {} recv buf len:{}", sock, buf_size);
		buf_size = SOCKET_RECV_BUF_LEN;
		if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)& buf_size, optlen) < 0) {
			LOG_ERR("setsockopt failed, error:{}", util::GetError());
			return INVALID_SOCKET_ID;
		}
		LOG_INF("Set udp socket {} recv buf len:{}", sock, buf_size);
	}
	
	if (getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)& buf_size, &optlen) < 0) {
		LOG_ERR("getsockopt failed, error:{}", util::GetError());
		return INVALID_SOCKET_ID;
	}
	else {
		LOG_INF("Get udp socket {} send buf len:{}", sock, buf_size);
		buf_size = SOCKET_SEND_BUF_LEN;
		if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)& buf_size, optlen) < 0) {
			LOG_ERR("setsockopt failed, error:{}", util::GetError());
			return INVALID_SOCKET_ID;
		}
		LOG_INF("Set udp socket {} send buf len:{}", sock, buf_size);
	}

	if (ep) { // udp listen need bind
		struct sockaddr_in sin;
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = GetSinAddr(*ep);
		sin.sin_port = htons(ep->port);
		if (::bind(sock, (sockaddr*)&sin, sizeof(sin)) < 0) {
			LOG_ERR("Bind socket failed, error:{}", util::GetError());
			evutil_closesocket(sock);
			return INVALID_SOCKET_ID;
		}
	}
	
	evutil_make_socket_nonblocking(sock); // non-blocking

	event* ev = event_new(m_ev_base, sock, EV_READ | EV_PERSIST, SocketCallback, this);
	if (!ev) {
		LOG_ERR("Create udp listen event failed!");
		return INVALID_SOCKET_ID;
	}
	event_add(ev, nullptr);

  m_mutex.lock();
	if (ep) { // udp listen
		m_sock_items.insert(std::make_pair(sock, UdpSockItem(ev, *ep)));
  }
  else {
    m_sock_items.insert(std::make_pair(sock, UdpSockItem(ev)));
  }
  m_mutex.unlock();

	return sock;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Socket UdpManager::CreateServerSocket(const com::Endpoint& ep)
{
	return CreateSocket(&ep);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Socket UdpManager::CreateClientSocket()
{
	return CreateSocket(nullptr);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void UdpManager::CloseSocket(Socket sock)
{
  m_mutex.lock();
	auto iter = m_sock_items.find(sock);
  if (iter != m_sock_items.end()) {
    event_del(iter->second.ev);
    LOG_INF("Remove udp socket:{}, ep:{}", sock, iter->second.ep.ToStr());
    m_sock_items.erase(iter);
  }
  else {
		LOG_ERR("Remove udp socket:{} failed!", sock);
	}
  m_mutex.unlock();

#ifdef _WINDOWS
	closesocket(sock);
#else
    close(sock);
#endif
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode UdpManager::SendData(Socket sock, const com::Endpoint& ep,
	com::Buffer buf)
{
	LOG_DBG("Send udp data, sock:{}, ep:{}, len:{}", sock, ep.ToStr(), 
    buf.data_len);

  m_mutex.lock();
  if (m_sock_items.find(sock) == m_sock_items.end()) {
    LOG_ERR("Cannot find udp sock:{} to send data!", sock);
    m_mutex.unlock();
    return com::ERR_CODE_FAILED;
  }
  m_mutex.unlock();

	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = GetSinAddr(ep);
	sin.sin_port = htons(ep.port);

	int res = sendto(sock, (char*)(buf.data.get() + buf.start_pos), buf.data_len, 
    0, (sockaddr*)&sin, sizeof(sin));
	if (res <= 0) {
		LOG_ERR("sendto failed!");
		return com::ErrCode::ERR_CODE_FAILED;
	}

	return com::ErrCode::ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void UdpManager::ThreadProc()
{
	LOG_INF("Enter udp manager thread");

	event_base_loop(m_ev_base, EVLOOP_NO_EXIT_ON_EMPTY);

	LOG_INF("Exit udp manager thread");
}

}

