#pragma once

#include "thread/event-thread.h"
#include "event.h"
#include "if-tcp-mgr.h"
#include "net-common.h"
#include "net-inner-message.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class TcpThread : public util::EventThread
{
public:
	TcpThread(ITcpHandler* handler);

	bool Init();

	void AddConnection(const com::Endpoint& lep, const com::Endpoint& rep, 
		SocketId sock);

	bool SendData(SocketId sock, com::Buffer buf);

	bool CloseConn(SocketId sock);

	void OnReadData(SocketId sock);

	void OnConnect(SocketId sock, short ev);

	// EventThread
	virtual void OnThreadMsg(const com::CommonMsg& msg) override;

private:
	void OnTcpConnect(TcpConnectMsgSP msg);
	void OnTcpConnectSuccess(const com::Endpoint& lep, const com::Endpoint& rep, 
		SocketId sock);
	void OnTcpConnectFailed(const com::Endpoint& lep, const com::Endpoint& rep, 
		SocketId sock);

	struct Connection
	{
		Connection(const com::Endpoint& l, const com::Endpoint& r, SocketId c, event* v)
			: lep(l), rep(r), sock(c), ev(v) {}

		//uint32_t conn_time;
		com::Endpoint lep;
		com::Endpoint rep;
		SocketId sock;
		event* ev = nullptr;
	};

private:
	std::mutex m_mutex;
	ITcpHandler* m_tcp_handler = nullptr;
	std::unordered_map<SocketId, Connection> m_conns;
};
typedef std::shared_ptr<TcpThread> TcpThreadSP;

}