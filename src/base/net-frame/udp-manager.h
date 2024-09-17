#pragma once

#include "event.h"
#include "if-udp-mgr.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "thread/common-thread.h"
#include "net-public.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class UdpManager
	: public base::ProxyUnknown
	, public base::ComObjTracer
	, public util::CommonThread
	, public IUdpMgr
{
public:
	UdpManager(base::IComFactory* factory, const char* owner);
	~UdpManager();

	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IUdpMgr
	virtual com::ErrCode Init(IUdpHandler* handler) override;
	virtual Socket CreateServerSocket(const com::Endpoint& ep) override;
	virtual Socket CreateClientSocket() override;
	virtual void CloseSocket(Socket sock) override;
	virtual com::ErrCode SendData(Socket sock, 
		const com::Endpoint& ep,
		com::Buffer buf) override;

	void OnReadData(Socket sock);

private:
	// CommonThread
	virtual void ThreadProc() override;

	struct UdpSockItem
	{
		UdpSockItem() {}
		UdpSockItem(event* v) : ev(v) {}
		UdpSockItem(event* v, const com::Endpoint& p) : ev(v), ep(p) {}

		event* ev = nullptr;
		com::Endpoint ep;
	};

	Socket CreateSocket(const com::Endpoint* ep);

private:
	event_base* m_ev_base = nullptr;

	IUdpHandler* m_udp_handler = nullptr;

	std::mutex m_mutex;

	std::unordered_map<Socket, UdpSockItem> m_sock_items;
};

}