// test-session-server.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <iostream>
#include <functional>

#include "udp-server.h"
#include "com-factory.h"
#include "net-message.h"
#include "common/util-net.h"
#include "common/util-time.h"

using namespace jukey::base;
using namespace jukey::util;
using namespace std::placeholders;


//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void UdpServer::OnRecvUdpData(const Endpoint& lep, const Endpoint& rep, 
  SocketId sock, Buffer buf)
{
	m_pkt_count++;
	m_recv_size += buf.data_len;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void UdpServer::OnSocketClosed(const Endpoint& lep, const Endpoint& rep,
  SocketId sock)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool UdpServer::Init(const std::string& addr, uint32_t log_level)
{
	std::optional<Address> result = ParseAddress(addr);
	if (result.has_value()) {
		m_addr = result.value();
	}
	else {
		std::cout << "Invalid address: " << addr << std::endl;
		return false;
	}

	IComFactory* factory = GetComFactory();
	if (!factory) {
		std::cout << "Get component factory failed!" << std::endl;
		return false;
	}

	if (!factory->Init("./")) {
		std::cout << "Init component factory failed!" << std::endl;
		return false;
	}

	m_udp_mgr = (IUdpMgr*)GetComFactory()->QueryInterface(CID_UDP_MGR,
		IID_UDP_MGR, "test");
	if (!m_udp_mgr) {
		std::cout << "Create udp manager failed!" << std::endl;
		return false;
	}

	if (ErrCode::ERR_CODE_OK != m_udp_mgr->Init(this)) {
		std::cout << "Init udp manager failed!" << std::endl;
		return false;
	}

	m_timer_mgr = (ITimerMgr*)factory->QueryInterface(CID_TIMER_MGR, 
		IID_TIMER_MGR, "udp manager");
	if (!m_timer_mgr) {
		std::cout << "Create timer manager failed!" << std::endl;
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void UdpServer::OnTimeout()
{
	std::cout << "######## recevied packet count:" << m_pkt_count
		<< " received packet size:" << m_recv_size << std::endl;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool UdpServer::Start()
{
	m_server_socket = m_udp_mgr->CreateServerSocket(m_addr.ep);
	if (m_server_socket <= 0) {
		std::cout << "Create udp server socket failed!" << std::endl;
		return false;
	}

	jukey::com::TimerParam timer_param;
	timer_param.timeout    = 1000;
	timer_param.timer_type = TimerType::TIMER_TYPE_LOOP;
	timer_param.timer_name = "udp server";
	timer_param.timer_func = [this](int64_t) {
		OnTimeout();
	};

	m_timer_id = m_timer_mgr->AllocTimer(timer_param);
	m_timer_mgr->StartTimer(m_timer_id);

	return true;
}
