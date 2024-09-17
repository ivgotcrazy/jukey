// test-session-client.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <thread>

#include "udp-client.h"
#include "com-factory.h"
#include "net-message.h"
#include "common/util-net.h"
#include "common/util-common.h"
#include "common/util-time.h"

using namespace jukey::base;
using namespace jukey::util;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SendUdpDataFunc(IUdpMgr* udp_mgr, Socket cs, Endpoint ep, uint32_t size, 
	uint32_t count, uint32_t bitrate)
{
	uint32_t loop = bitrate / 10 / size;
	if (loop <= 0) {
		std::cout << "Invalid loop!!!" << std::endl;
		exit(1);
	}

	Buffer buf(size);
	buf.data_len = size;
	memset(buf.data.get(), 0, buf.data_len);

	uint64_t total_send = 0;
	for (uint32_t i = 0; i < count;) {
		for (uint32_t j = 0; j < loop; j++) {
			if (udp_mgr->SendData(cs, ep, buf) != ErrCode::ERR_CODE_OK) {
				printf("Send udp data failed!\n");
			}
			total_send += size;
		}
		i += loop;

		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		if (i % 100 == 0) {
			printf("<<< Send packet count:%d, packet size: %lld\n", i, total_send);
		}
	}

	printf("<<< Send packet count:%d, packet size: %lld\n", count, total_send);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void UdpClient::OnRecvUdpData(const Endpoint& lep, const Endpoint& rep,
	SocketId sock, Buffer buf)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void UdpClient::OnSocketClosed(const Endpoint& lep, const Endpoint& rep,
	SocketId sock)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool UdpClient::Init(const ClientParam& param)
{
	std::optional<Address> result = ParseAddress(param.addr);
	if (result.has_value()) {
		m_addr = result.value();
	}
	else {
		std::cout << "Invalid address: " << param.addr << std::endl;
		return false;
	}

  m_param = param;

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

	m_client_socket = m_udp_mgr->CreateClientSocket();
	if (m_client_socket <= 0) {
		std::cout << "Create udp client socket failed!" << std::endl;
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool UdpClient::Start()
{
	std::thread t(SendUdpDataFunc, 
		m_udp_mgr, 
		m_client_socket, 
		m_addr.ep,
		m_param.packet_size, 
		m_param.packet_count, 
		m_param.bitrate);
	t.detach();

	return true;
}