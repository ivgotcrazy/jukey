#include <regex>
#include <future>

#include "net-inner-message.h"
#include "session-mgr.h"
#include "com-factory.h"
#include "server-session.h"
#include "client-session.h"
#include "common/util-common.h"
#include "common/util-time.h"
#include "common-config.h"
#include "log.h"

using namespace jukey::util;
using namespace jukey::com;

namespace jukey::net
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionMgr::SessionMgr(base::IComFactory* factory, const char* owner)
	: ProxyUnknown(nullptr)
  , ComObjTracer(factory, CID_SESSION_MGR, owner)
	, CommonThread("SessionMgr", 4096 * 1024, true)
	, m_factory(factory)
{
	assert(m_factory);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* SessionMgr::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_SESSION_MGR) == 0) {
		return new SessionMgr(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* SessionMgr::NDQueryInterface(const char* riid)
{
	if (0 == strcmp(riid, IID_SESSION_MGR)) {
		return static_cast<ISessionMgr*>(this);
	}
	else {
		return ProxyUnknown::NDQueryInterface(riid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SessionMgr::CheckSessionMgrParam(const SessionMgrParam& param)
{
	if (param.ka_interval == 0 || param.ka_interval > 31) {
		LOG_ERR("Invalid keep alive interval:{}", param.ka_interval);
		return false;
	}

	if (param.thread_count == 0 || param.thread_count > 1024) {
		LOG_ERR("Invalid thread count!");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode SessionMgr::Init(const SessionMgrParam& param)
{
	if (!CheckSessionMgrParam(param)) {
		return ERR_CODE_INVALID_PARAM;
	}
	else {
		m_mgr_param = param;
	}

	m_tcp_mgr = (ITcpMgr*)QI(CID_TCP_MGR, IID_TCP_MGR, "session manager");
	if (!m_tcp_mgr) {
		LOG_ERR("Create tcp manager failed!");
		return ERR_CODE_FAILED;
	}

	// TCP manager thread count keep pace with session manager thread count
	if (ERR_CODE_OK != m_tcp_mgr->Init(this, 
		param.thread_count)) {
		LOG_ERR("Init tcp manager failed!")
		return ERR_CODE_FAILED;
	}

	m_udp_mgr = (IUdpMgr*)QI(CID_UDP_MGR, IID_UDP_MGR, "session manager");
	if (!m_udp_mgr) {
		LOG_ERR("Create udp manager failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != m_udp_mgr->Init(this)) {
		LOG_ERR("Init udp manager failed!");
		return ERR_CODE_FAILED;
	}

	for (uint32_t i = 0; i < param.thread_count; i++) {
		SessionThreadSP th(new SessionThread(m_factory, i));
		th->Start();
		m_session_threads.push_back(th);
	}

	StartThread();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// Cross-thread synchronization to obtain available session identifiers
//------------------------------------------------------------------------------
SessionId SessionMgr::GetSessionIdFromSt(uint32_t thread_index)
{
	FetchSessionIdMsgSP data(new FetchSessionIdMsg(
		(uint32_t)m_session_threads.size(), thread_index));

	com::CommonMsg msg;
	msg.msg_type = NET_INNER_MSG_ALLOC_SESSION_ID;
	msg.msg_data = data;

	m_session_threads[thread_index]->PostMsg(msg);

	return data->sid.get_future().get();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionId SessionMgr::GetAvailableSessionId()
{
	std::lock_guard<std::mutex> lock(m_sid_mutex);

	if (m_session_index < 0xFFFF) {
		++m_session_index;
		if (m_session_index == 0xFFFF) {
			LOG_WRN("Reach max session index!");
		}
		return m_session_index;
	}
	else {
		for (uint32_t i = 0; i <= m_session_threads.size(); i++) {
			if (INVALID_SESSION_ID != GetSessionIdFromSt(i)) {
				return i;
			}
		}
		LOG_ERR("Allocate session failed!");
		return INVALID_SESSION_ID;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SessionMgr::CheckCreateSessionParam(const CreateParam& param)
{
  if (!CheckAddress(param.remote_addr)) {
    LOG_ERR("Invalid address:{}", param.remote_addr.ToStr());
    return false;
  }

  if (param.service_type == com::ServiceType::INVALID) {
    LOG_ERR("Invalid service type!");
    return false;
  }

  if (!param.thread) {
    LOG_ERR("Invalid thread!");
    return false;
  }

  if (param.remote_addr.type == AddrType::TCP) {
	  if (param.session_type == SessionType::UNRELIABLE) {
		  LOG_ERR("Tcp address must use RELIABLE session type!");
		  return false;
	  }

	  if (param.fec_type != FecType::NONE) {
		  LOG_ERR("Tcp address cannot set FEC!");
		  return false;
	  }
  }

  return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionId SessionMgr::CreateSession(const CreateParam& param)
{
	LOG_INF("Create session, addr:{}, inteval:{}, reliable:{}",
		param.remote_addr.ToStr(), param.ka_interval, param.session_type);

  if (!CheckCreateSessionParam(param)) {
    return INVALID_SESSION_ID;
  }

	std::lock_guard<std::mutex> lock(m_mutex);

	SessionId sid = GetAvailableSessionId();
	if (sid == INVALID_SESSION_ID) {
		return INVALID_SESSION_ID;
	}

	if (param.remote_addr.type == com::AddrType::TCP) {
		if (ERR_CODE_OK != m_tcp_mgr->Connect(
			param.remote_addr.ep)) {
			LOG_ERR("Connect to {} failed!", param.remote_addr.ToStr());
			return INVALID_SESSION_ID;
		}
		m_conn_que.push_back(ConnItem(param, sid));
	}
	else if (param.remote_addr.type == com::AddrType::UDP) {
		SocketId sock = m_udp_mgr->CreateClientSocket();
		if (sock == INVALID_SOCKET_ID) {
			LOG_ERR("Create udp client socket failed!");
			return INVALID_SESSION_ID;
		}
		AddClientSession(com::AddrType::UDP, sock, sid, param);
		LOG_INF("Create client udp socket {}", sock);
	}

	return sid;
}

//------------------------------------------------------------------------------
// 从 Session 调用过来
//------------------------------------------------------------------------------
void SessionMgr::InnerCloseSession(SessionId sid)
{
	LOG_INF("InnerCloseSession:{}", sid);

	std::lock_guard<std::mutex> lock(m_mutex);
	
	DoCloseSession(sid, false);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SessionMgr::DoRemoveSession(SessionId sid)
{
	bool found = false;

	// Tcp session first
	for (auto iter = m_tcp_map.begin(); iter != m_tcp_map.end(); iter++) {
		if (iter->second == sid) {
			m_tcp_map.erase(iter);
			LOG_INF("Remove session {} from tcp map", sid);
			found = true;
			break;
		}
	}

	// Then udp session
	if (!found) {
		for (auto iter = m_udp_map.begin(); iter != m_udp_map.end(); iter++) {
			if (iter->second == sid) {
				m_udp_map.erase(iter);
				LOG_INF("Remove session {} from udp map", sid);
				found = true;
				break;
			}
		}
	}

	if (!found) {
		LOG_ERR("Cannot find session {} to close!", sid);
	}

	return found;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode SessionMgr::DoCloseSession(SessionId sid, bool active)
{
	if (!DoRemoveSession(sid)) {
		return ERR_CODE_SESSION_NOT_EXIST;
	}
	
	PostRemoveSessionMsg(sid, active);

	LOG_INF("Total session count:{}", m_tcp_map.size() + m_udp_map.size());

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// FIXME: 关闭会话需要使用同步语义，否则可能出现调用者已经析构，但是会话还会回调的情况
//------------------------------------------------------------------------------
com::ErrCode SessionMgr::CloseSession(SessionId sid)
{
	LOG_INF("Close session {}", sid);

	std::lock_guard<std::mutex> lock(m_mutex);

	return DoCloseSession(sid, true);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ListenId SessionMgr::GetAvailableListenId()
{
	if (m_listen_index < 0xFFFF) {
		++m_listen_index;
		if (m_listen_index == 0xFFFF) {
			LOG_WRN("Reach max session index!");
		}
		LOG_INF("Alloc listen ID:{}", m_listen_index);
		return m_listen_index;
	}
	else {
		for (uint16_t i = 1; i < 0xFFFF; i++) {
			if (m_listen_map.find(i) == m_listen_map.end()) {
				LOG_INF("Alloc listen ID:{}", i);
				return i;
			}
		}
		LOG_ERR("Allocate session failed!");
		return INVALID_SESSION_ID;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ListenId SessionMgr::AddListen(const ListenParam& param)
{
	LOG_INF("Add listen {}:{}:{}", 
		param.listen_addr.type == com::AddrType::TCP ? "TCP" : "UDP",
		param.listen_addr.ep.host,
		param.listen_addr.ep.port);

	if (!CheckAddress(param.listen_addr)) {
		LOG_ERR("Invalid address:{}", param.listen_addr.ToStr());
		return INVALID_LISTEN_ID;
	}

	if (param.listen_srv == com::ServiceType::INVALID) {
		LOG_ERR("Invalid service type!");
		return INVALID_LISTEN_ID;
	}

	if (!param.thread) {
		LOG_ERR("Invalid thread!");
		return INVALID_LISTEN_ID;
	}

	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto item : m_listen_map) {
		if (item.second.param.listen_addr == param.listen_addr) {
			LOG_ERR("Address already on lisen!");
			return INVALID_LISTEN_ID;
		}
	}

	ListenId listen_id = GetAvailableListenId();
	if (listen_id == INVALID_LISTEN_ID) {
		return INVALID_LISTEN_ID;
	}

	if (param.listen_addr.type == AddrType::TCP) {
		if (ERR_CODE_OK != m_tcp_mgr->AddListen(param.listen_addr.ep)) {
			LOG_ERR("Add tcp listen failed!");
			return INVALID_LISTEN_ID;
		}
		m_listen_map.insert(std::make_pair(listen_id, 
			ListenItem(param, INVALID_SOCKET_ID)));
	}
	else if (param.listen_addr.type == AddrType::UDP) {
		SocketId sock = m_udp_mgr->CreateServerSocket(param.listen_addr.ep);
		if (sock == INVALID_SOCKET_ID) {
			LOG_ERR("Create udp server socket failed!");
			return INVALID_LISTEN_ID;
		}
		LOG_INF("Create server udp socket {}", sock);
		m_listen_map.insert(std::make_pair(listen_id, ListenItem(param, sock)));
	}
	else {
		LOG_ERR("Invalid transport type {}", param.listen_addr.type);
		return INVALID_LISTEN_ID;
	}

	return listen_id;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode SessionMgr::RemoveListen(ListenId lid)
{
	LOG_INF("Remove listen ID:{}", lid);

	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_listen_map.find(lid);
	if (iter == m_listen_map.end()) {
		LOG_ERR("Cannot find listen ID:{}", lid);
		return ERR_CODE_FAILED;
	}

	if (iter->second.param.listen_addr.type == com::AddrType::TCP) {
		m_listen_map.erase(iter);
		if (ERR_CODE_OK != m_tcp_mgr->RemoveListen(
			iter->second.param.listen_addr.ep)) {
			LOG_ERR("Remove tcp listen failed!");
			return ERR_CODE_FAILED;
		}
	}
	else if (iter->second.param.listen_addr.type == com::AddrType::UDP) {
		m_listen_map.erase(iter);
		m_udp_mgr->CloseSocket(iter->second.sock);
	}
	else {
		LOG_ERR("Invalid transport type {}", iter->second.param.listen_addr.type);
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode SessionMgr::SendData(SessionId sid, const com::Buffer& buf)
{
  LOG_DBG("[session:{}] Send data, len:{}", sid, buf.data_len);

	if (buf.data_len > SEND_DATA_MAX_SIZE) {
		LOG_ERR("Invalid send data length:{}", buf.data_len);
		return ERR_CODE_FAILED;
	}

	SendSessionDataMsgSP data(new SendSessionDataMsg(sid, buf));

	com::CommonMsg msg;
	msg.msg_type = NET_INNER_MSG_SEND_SESSION_DATA;
	msg.msg_data = data;

	if (GetSessionThread(sid)->PostMsg(msg)) {
		return ERR_CODE_OK;
	}
	else {
		return ERR_CODE_FAILED; // FIXME: 错误码重新定义PENDING
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionThreadSP SessionMgr::GetSessionThread(SessionId sid)
{
	return m_session_threads[sid % m_session_threads.size()];
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionMgr::DoAddSession(ISessionSP session, com::AddrType addr_type,
	SocketId sock, SessionId sid, const com::Endpoint& rep)
{
	if (addr_type == com::AddrType::TCP) {
		m_tcp_map.insert(std::make_pair(sock, sid));
	}
	else if (addr_type == com::AddrType::UDP) {
		m_udp_map.insert(std::make_pair(UdpConn(sock, rep), sid));
	}
	else {
		LOG_ERR("Invalid transport type {}", addr_type);
		return;
	}

	LOG_INF("[session:{}] Post add session, tcp map count:{}, udp map count:{}", 
		sid, m_tcp_map.size(), m_udp_map.size());

	GetSessionThread(sid)->PostMsg(CommonMsg(NET_INNER_MSG_ADD_SESSION, session));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionMgr::AddClientSession(com::AddrType addr_type, SocketId sock,
	SessionId sid, const CreateParam& param)
{
	LOG_INF("Add client session, type:{}, socket:{}, sid:{}, addr:{}", 
		addr_type, sock, sid, param.remote_addr.ToStr());

  SessionParam sparam;
  sparam.local_kai    = param.ka_interval;
  sparam.local_sid    = sid;
  sparam.remote_sid   = 0;
  sparam.peer_seen_ip = 0;
  sparam.remote_addr  = param.remote_addr;
  sparam.service_type = param.service_type;
  sparam.session_type = param.session_type;
  sparam.session_role = SessionRole::CLIENT;
  sparam.sock         = sock;
  sparam.thread       = param.thread;
  sparam.fec_type     = param.fec_type;

	SessionThreadSP st = GetSessionThread(sid);
	ISessionSP session(new ClientSession(this, st, sparam, param));
	DoAddSession(session, addr_type, sock, sid, param.remote_addr.ep);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionMgr::AddServerSession(AddrType addr_type, const Endpoint& lep, 
  const Endpoint& rep, SocketId sock, SessionId sid)
{
	LOG_INF("Add server session, type:{}, socket:{}, sid:{}, endpoint:{}",
		addr_type, sock, sid, rep.ToStr());

	ListenItem item = GetListenItemByEndpoint(lep);
	if (item.param.listen_srv == com::ServiceType::INVALID) {
		LOG_ERR("Cannot find listen item by local endpoint:{}", lep.ToStr());
		return;
	}

  SessionParam sparam;
  sparam.remote_addr  = Address(rep, addr_type);
  sparam.peer_seen_ip = 0;
  sparam.local_kai    = m_mgr_param.ka_interval;
  sparam.remote_kai   = 0;
  sparam.service_type = item.param.listen_srv;
  sparam.session_type = SessionType::INVALID;
  sparam.thread       = item.param.thread;
  sparam.local_sid    = sid;
  sparam.remote_sid   = 0;
  sparam.sock         = sock;
	sparam.session_role = SessionRole::SERVER;

	SessionThreadSP st = GetSessionThread(sid);
	ISessionSP session(new ServerSession(this, st, sparam, m_mgr_param));	
	DoAddSession(session, addr_type, sock, sid, rep);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionMgr::OnIncommingConn(const Endpoint& lep, const Endpoint& rep, 
  SocketId sock)
{
	LOG_INF("Incomming connection, local endpoint:{}, remote endpoint:{}, socket:{}",
		lep.ToStr(), rep.ToStr(), sock);

	std::lock_guard<std::mutex> lock(m_mutex);

	SessionId sid = GetAvailableSessionId();
	if (sid != INVALID_SESSION_ID) {
		AddServerSession(com::AddrType::TCP, lep, rep, sock, sid);
	}
}

//------------------------------------------------------------------------------
// Unify TCP and UDP to find sessions via endpoint
//------------------------------------------------------------------------------
void SessionMgr::OnConnectResult(const Endpoint& lep, const Endpoint& rep, 
  SocketId sock, bool result)
{
	LOG_INF("Connect to {} result:{}, socket:{}", rep.ToStr(), result, sock);

	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto iter = m_conn_que.begin(); iter != m_conn_que.end(); iter++) {
		if (iter->param.remote_addr.ep == rep) {
			if (result) {
				AddClientSession(com::AddrType::TCP, sock, iter->sid, iter->param);
			}
			else {
				com::CommonMsg msg;
				msg.msg_type = NET_MSG_SESSION_CREATE_RESULT;
				msg.msg_data.reset(new SessionCreateResultMsg(
          iter->sid,
          INVALID_SESSION_ID, // FIXME:
          false, 
          Address(rep, AddrType::UDP)));
				iter->param.thread->PostMsg(msg);
			}
			m_conn_que.erase(iter);
			LOG_INF("Remove tcp connecting item {}", rep.ToStr());
			return;
		}
	}

	LOG_ERR("Cannot find session!");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionMgr::OnConnClosed(const Endpoint& lep, const Endpoint& rep, 
	SocketId sock)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_tcp_map.find(sock);
	if (iter == m_tcp_map.end()) {
		LOG_ERR("Cannot find session by socket:{}", sock);
	}
	else {
		DoCloseSession(iter->second, false);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionMgr::PostConnectionDataMsg(SessionId sid, const com::Buffer& buf)
{
	LOG_DBG("[session:{}] Post session data, len:{}", sid, buf.data_len);

	ConnectionDataMsgSP data(new ConnectionDataMsg(sid, buf));
	com::CommonMsg msg;
	msg.msg_data = data;
	msg.msg_type = NET_INNER_MSG_RECV_SESSION_DATA;

	GetSessionThread(sid)->PostMsg(msg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionMgr::OnRecvTcpData(const Endpoint& lep, const Endpoint& rep,
	SocketId sock, com::Buffer buf)
{
	LOG_DBG("Received tcp data, local:{}, remote:{}, len:{}", lep.ToStr(), 
		rep.ToStr(), buf.data_len);

	std::lock_guard<std::mutex> lock(m_mutex);

	// TODO: inefficient
	auto iter = m_tcp_map.find(sock);
	if (iter == m_tcp_map.end()) {
		LOG_ERR("Cannot find session by socket: {}!", sock);
		return;
	}

	PostConnectionDataMsg(iter->second, buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionMgr::PostRemoveSessionMsg(SessionId sid, bool active)
{
	com::CommonMsg msg;
	msg.msg_type = NET_INNER_MSG_REMOVE_SESSION;
	msg.msg_data.reset(new RemoveSessionMsg(sid, active));

	GetSessionThread(sid)->PostMsg(msg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SessionMgr::ListenItem SessionMgr::GetListenItemByEndpoint(const Endpoint& ep)
{
	for (auto item : m_listen_map) {
		if (item.second.param.listen_addr.ep == ep) {
			return item.second;
		}
	}
	return ListenItem();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionMgr::OnRecvUdpData(const Endpoint& lep, const Endpoint& rep,
	SocketId sock, com::Buffer buf)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_udp_map.find(UdpConn(sock, rep));
	if (iter == m_udp_map.end()) { // new session
		SessionId sid = GetAvailableSessionId();
		if (sid == INVALID_SESSION_ID) {
			LOG_ERR("Get avaliable session ID failed!");
		}
		else {
			AddServerSession(com::AddrType::UDP, lep, rep, sock, sid);
			PostConnectionDataMsg(sid, buf);
		}
	}
	else { // old session
		PostConnectionDataMsg(iter->second, buf);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionMgr::OnSocketClosed(const Endpoint& lep, const Endpoint& rep,
	SocketId sock)
{
	LOG_DBG("Udp socket closed, local:{}, remote:{}", lep.ToStr(), rep.ToStr());

	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_udp_map.find(UdpConn(sock, rep));
	if (iter != m_udp_map.end()) {
		DoCloseSession(iter->second, false);
	}
	else {
		LOG_DBG("Cannot find session to remove!");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionMgr::OnThreadMsg(const com::CommonMsg& msg)
{
	//switch (msg.msg_type) {
	//default:
	//	LOG_ERR("Unexpected message {}", msg.msg_type);
	//}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SessionMgr::SetLogLevel(uint8_t level)
{
  g_net_logger->SetLogLevel(level);
}

}