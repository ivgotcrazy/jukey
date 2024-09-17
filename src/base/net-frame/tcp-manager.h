#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "event.h"

#include "thread/common-thread.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "if-tcp-mgr.h"
#include "tcp-thread.h"
#include "net-public.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class TcpManager
	: public base::ProxyUnknown
  , public base::ComObjTracer
	, public util::CommonThread
	, public ITcpMgr
{
public:
	TcpManager(base::IComFactory* factory, const char* owner);
	~TcpManager();

	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IConnectionMgr
	virtual com::ErrCode Init(ITcpHandler* handler, uint32_t thread_count) override;
	virtual com::ErrCode AddListen(const com::Endpoint& ep) override;
	virtual com::ErrCode RemoveListen(const com::Endpoint& ep) override;
	virtual com::ErrCode Connect(const com::Endpoint& ep) override;
	virtual com::ErrCode SendData(SocketId sock, com::Buffer buf) override;
	virtual com::ErrCode CloseConn(SocketId sock) override;

	// CommonThread
	virtual void ThreadProc() override;

	void OnIncommingConnection(SocketId sock);

private:
	bool IsAlreadyOnListen(const com::Endpoint& ep);

	struct TcpListenItem
	{
		TcpListenItem() {}
		TcpListenItem(const com::Endpoint& e, event* v) : ep(e), ev(v) {}

		com::Endpoint ep;
		event* ev = nullptr;
	};

	void SetSocketBuffer(Socket sock);

private:
	std::unordered_map<SocketId, TcpListenItem> m_listen_items;

	// TODO: 使用std::vector<ConnectionThreadSP>会报错
	//TcpThreadSP* m_conn_threads = nullptr;
	std::vector<TcpThreadSP> m_conn_threads;

	event_base* m_ev_base = nullptr;

	ITcpHandler* m_tcp_handler = nullptr;

	uint32_t m_thread_count = 4;

	std::mutex m_mutex;
};

}