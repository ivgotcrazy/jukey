#include "tcp-manager.h"
#include "net-message.h"
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
void AcceptHandler(Socket socket, short ev, void* data)
{
	TcpManager* mgr = static_cast<TcpManager*>(data);
	mgr->OnIncommingConnection(socket);
}

}

using namespace jukey::com;

namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
TcpManager::TcpManager(base::IComFactory* factory, const char* owner)
	: base::ProxyUnknown(nullptr)
  , base::ComObjTracer(factory, CID_TCP_MGR, owner)
	, CommonThread("TcpManager", true)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
TcpManager::~TcpManager()
{
	if (m_ev_base) {
		event_base_free(m_ev_base);
		m_ev_base = nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* TcpManager::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
  if (strcmp(cid, CID_TCP_MGR) == 0) {
    return new TcpManager(factory, owner);
  }
  else {
    LOG_ERR("Invalid cid {}", cid);
    return nullptr;
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* TcpManager::NDQueryInterface(const char* riid)
{
  if (0 == strcmp(riid, IID_TCP_MGR)) {
    return static_cast<ITcpMgr*>(this);
  }
  else {
    return ProxyUnknown::NDQueryInterface(riid);
  }
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode TcpManager::Init(ITcpHandler* handler, uint32_t thread_count)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (thread_count == 0) {
		LOG_ERR("Thread cound cannot be zero!");
		return ERR_CODE_FAILED;
	}

#ifdef _WINDOWS
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		LOG_ERR("WSAStartup failed!");
		return ERR_CODE_FAILED;
	}

	evthread_use_windows_threads();
#endif

	m_thread_count = thread_count;
	m_tcp_handler = handler;

	for (uint32_t i = 0; i < m_thread_count; i++) {
		TcpThreadSP thread(new TcpThread(handler));
		m_conn_threads.push_back(thread);
		thread->Init();
	}

	m_ev_base = event_base_new();
	if (!m_ev_base) {
		LOG_ERR("Create event base failed!");
		return ERR_CODE_FAILED;
	}

	StartThread();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TcpManager::ThreadProc()
{
	LOG_INF("Enter tcp manager thread");

	event_base_loop(m_ev_base, EVLOOP_NO_EXIT_ON_EMPTY);

	LOG_INF("Exit tcp manager thread");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool TcpManager::IsAlreadyOnListen(const com::Endpoint& ep)
{
  std::lock_guard<std::mutex> lock(m_mutex);

	for (auto item : m_listen_items) {
		if (item.second.ep == ep) {
			return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode TcpManager::AddListen(const com::Endpoint& ep)
{
	if (IsAlreadyOnListen(ep)) {
		LOG_WRN("Address {}:{} is already on listen!", ep.host, ep.port);
		return ERR_CODE_FAILED;
	}

	Socket listener = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listener == -1) {
		LOG_ERR("Create socket failed, error = {}", util::GetError());
		return ERR_CODE_FAILED;
	}

	evutil_make_listen_socket_reuseable(listener); // call before bind

	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = (ep.host.length() == 0) ? 0 : inet_addr(ep.host.c_str());
	sin.sin_port = htons(ep.port);

	if (::bind(listener, (sockaddr*)& sin, sizeof(sin)) < 0) {
		LOG_ERR("Bind socket failed!");
		evutil_closesocket(listener);
		return ERR_CODE_FAILED;
	}

	// TODO: backlog
	if (::listen(listener, 10) < 0) {
		LOG_ERR("Listen failed!");
		return ERR_CODE_FAILED;
	}

	evutil_make_socket_nonblocking(listener); // non-blocking

	event* ev = event_new(m_ev_base, listener, EV_READ | EV_PERSIST, AcceptHandler, this);
	if (!ev) {
		LOG_ERR("Create listen event failed!");
		return ERR_CODE_FAILED;
	}
	event_add(ev, nullptr);

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listen_items.insert(std::make_pair(listener, TcpListenItem(ep, ev)));
  }

	LOG_INF("Add listen to {}:{} success", ep.host, ep.port);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode TcpManager::RemoveListen(const com::Endpoint& ep)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto iter : m_listen_items) {
		if (iter.second.ep == ep) {
			if (event_del(iter.second.ev) < 0) {
				LOG_ERR("Remove event failed!");
				break;
			}
			m_listen_items.erase(iter.first);
			return ERR_CODE_OK;
		}
	}

	return ERR_CODE_FAILED;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode TcpManager::Connect(const com::Endpoint& ep)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	Socket sock = ::socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		LOG_ERR("Create socket failed!");
		return ERR_CODE_FAILED;
	}
	SetSocketBuffer(sock);

	com::CommonMsg msg;
	msg.msg_type = NET_INNER_MSG_TCP_CONNECT;
	msg.msg_data.reset(new TcpConnectMsg(ep, sock));

	m_conn_threads[sock % m_thread_count]->PostMsg(msg);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode TcpManager::SendData(SocketId sock, com::Buffer buf)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	return m_conn_threads[sock % m_thread_count]->SendData(sock, buf) ?
		ERR_CODE_OK : ERR_CODE_FAILED;
}

//------------------------------------------------------------------------------
// TODO:
//------------------------------------------------------------------------------
com::ErrCode TcpManager::CloseConn(SocketId sock)
{
	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TcpManager::SetSocketBuffer(Socket sock)
{
	int buf_size = -1;
	socklen_t optlen = sizeof(buf_size);
	if (getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&buf_size, &optlen) < 0)
	{
		LOG_ERR("getsockopt error:{}", util::GetError());
		return;
	}

	LOG_INF("Get tcp socket {} recv buf len:{}", sock, buf_size);
	buf_size = SOCKET_RECV_BUF_LEN;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&buf_size, optlen) < 0) {
		LOG_ERR("setsockopt error:{}", util::GetError());
		return;
	}
	LOG_INF("Set tcp socket {} recv buf len:{}", sock, buf_size);


	if (getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&buf_size, &optlen) < 0) {
		LOG_ERR("getsockopt error:{}", util::GetError());
		return;
	}

	LOG_INF("Get tcp socket {} send buf len:{}", sock, buf_size);
	buf_size = SOCKET_SEND_BUF_LEN;
	if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&buf_size, optlen) < 0) {
		LOG_ERR("setsockopt error:{}", util::GetError());
		return;
	}
	LOG_INF("Set tcp socket {} send buf len:{}", sock, buf_size);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TcpManager::OnIncommingConnection(SocketId sock)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);
	Socket client_sock = ::accept(sock, (struct sockaddr*)(&client_addr), &len);
	if (client_sock <= 0) {
		LOG_ERR("Socket {} accept failed!", sock);
		return;
	}
	SetSocketBuffer(client_sock);

	auto iter = m_listen_items.find(sock);
	if (iter == m_listen_items.end()) {
		LOG_ERR("Cannot find listen socket!");
		return;
	}

	com::Endpoint& lep = m_listen_items[sock].ep;
	com::Endpoint rep = GetAddress(client_addr);
	
	LOG_INF("Incomming socket:{}, listen:{}, remote:{}", client_sock,
		lep.ToStr(), rep.ToStr());

	// Notify incomming connection
	m_tcp_handler->OnIncommingConn(lep, rep, client_sock);

	// Put connection to tcp thread
	m_conn_threads[sock % m_thread_count]->AddConnection(lep, rep, client_sock);
}

}
