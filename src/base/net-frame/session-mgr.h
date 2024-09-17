#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

#include "if-session.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "if-session-mgr.h"
#include "if-tcp-mgr.h"
#include "if-udp-mgr.h"
#include "session-thread.h"
#include "net-common.h"
#include "common-error.h"
#include "thread/common-thread.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class SessionMgr 
	: public base::ProxyUnknown
  , public base::ComObjTracer
	, public ISessionMgr
	, public ITcpHandler
	, public IUdpHandler
	, public util::CommonThread
{
public:
	SessionMgr(base::IComFactory* factory, const char* owner);

	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// ISessionMgr
	virtual com::ErrCode Init(const SessionMgrParam& param) override;
	virtual SessionId CreateSession(const CreateParam& param) override;
	virtual com::ErrCode CloseSession(SessionId sid) override;
	virtual ListenId AddListen(const ListenParam& param) override;
	virtual com::ErrCode RemoveListen(ListenId lid) override;
	virtual com::ErrCode SendData(SessionId sid, const com::Buffer& buf) override;
	virtual void SetLogLevel(uint8_t level) override;

	base::IComFactory* GetComFactory() { return m_factory; }

private:
	struct ConnItem
	{
		ConnItem(CreateParam p, SessionId s) : param(p), sid(s) {}

		CreateParam param;
		SessionId sid = INVALID_SESSION_ID;
	};

	struct ListenItem
	{
		ListenItem() {}
		ListenItem(const ListenParam& p, SocketId c) : param(p), sock(c) {}

		ListenParam param;
		SocketId sock = INVALID_SOCKET_ID; // Socket: for udp
	};

	struct UdpConn
	{
		UdpConn(SocketId c, const com::Endpoint& ep) : sock(c), rep(ep) {}

		SocketId sock;
		com::Endpoint rep;

		bool operator==(const UdpConn& conn) const
		{
			return sock == conn.sock && rep == conn.rep;
		}
	};

	struct UdpConnHash
	{
		size_t operator()(const UdpConn& conn) const
		{
			return std::hash<std::string>()(
				conn.rep.ToStr().append(std::to_string(conn.sock)));
		}
	};

private:
	// ITcpHandler
	virtual void OnIncommingConn(const com::Endpoint& lep, 
		const com::Endpoint& rep,
		SocketId sock) override;
	virtual void OnConnectResult(const com::Endpoint& lep,
		const com::Endpoint& rep,
		SocketId sock, 
		bool result) override;
	virtual void OnConnClosed(const com::Endpoint& lep,
		const com::Endpoint& rep,
		SocketId sock) override;
	virtual void OnRecvTcpData(const com::Endpoint& lep,
		const com::Endpoint& rep,
		SocketId sock, 
		com::Buffer buf) override;

	// IUdpHandler
	virtual void OnRecvUdpData(const com::Endpoint& lep,
		const com::Endpoint& rep,
		SocketId sock,
		com::Buffer buf) override;
	virtual void OnSocketClosed(const com::Endpoint& lep,
		const com::Endpoint& rep,
		SocketId sock) override;

	// CommonThread
	virtual void OnThreadMsg(const com::CommonMsg& msg) override;

private:
	SessionThreadSP GetSessionThread(SessionId sid);
	SessionId GetAvailableSessionId();
	SessionId GetSessionIdFromSt(uint32_t thread_index);
	void PostConnectionDataMsg(SessionId sid, const com::Buffer& buf);
	void PostRemoveSessionMsg(SessionId sid, bool active);
	bool CheckSessionMgrParam(const SessionMgrParam& param);
	bool CheckCreateSessionParam(const CreateParam& param);
	ListenItem GetListenItemByEndpoint(const com::Endpoint& ep);
	com::ErrCode DoCloseSession(SessionId sid, bool active);
	ListenId GetAvailableListenId();
	void InnerCloseSession(SessionId sid);
	bool DoRemoveSession(SessionId sid);

	void AddClientSession(com::AddrType addr_type,
		SocketId sock, 
		SessionId sid, 
		const CreateParam& param);
	void AddServerSession(com::AddrType addr_type,
		const com::Endpoint& lep,
		const com::Endpoint& rep,
		SocketId sock,
		SessionId sid);
	void DoAddSession(ISessionSP session, 
		com::AddrType addr_type,
		SocketId sock, 
		SessionId sid, 
		const com::Endpoint& rep);
	
private:
	friend class SessionBase;
	friend class ClientSession;
	friend class ServerSession;
	
  // Listen
	std::unordered_map<ListenId, ListenItem> m_listen_map;

  // Multi-thread support
	std::vector<SessionThreadSP> m_session_threads;

	// For TCP session finding acceleration
	std::unordered_map<SocketId, SessionId> m_tcp_map;

	// For UDP session finding acceleration
	std::unordered_map<UdpConn, SessionId, UdpConnHash> m_udp_map;

	// Session negotiation waiting queue
	std::vector<ConnItem> m_conn_que;

	// Transporting
	ITcpMgr* m_tcp_mgr = nullptr;
	IUdpMgr* m_udp_mgr = nullptr;

	std::mutex m_mutex;

	// Session ID allocation
	uint16_t m_session_index = INVALID_SESSION_ID;

	// Listen ID allocation
	uint16_t m_listen_index = INVALID_LISTEN_ID;

	// Session ID allocation mutex
	std::mutex m_sid_mutex;

	SessionMgrParam m_mgr_param;

	base::IComFactory* m_factory = nullptr;
};

}