// test-session-client.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

#include "udp-client.h"
#include "session-client.h"
#include "clipp.h"
#include "common/util-time.h"


using namespace clipp;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
int main(int argc, char** argv)
{
	ClientParam param;
	param.reliable = false; // 默认不可靠

	auto cli = (value("{TCP|UDP]}:IP:Port", param.addr),
		option("-s", "") & value("packet size", param.packet_size),
		option("-c", "") & value("send packet count", param.packet_count),
		option("-b", "") & value("send bitrate", param.bitrate),
		option("-p", "") & value("send packet rate", param.packet_rate),
		option("-w", "").set(param.raw).doc("socket mode, else session mode"),
		option("-a", "").set(param.reliable).doc("reliable session"),
		option("-f", "").set(param.fec).doc("use fec"),
		option("-l", "") & value("log level", param.log_level)
	);

	if (!parse(argc, argv, cli)) {
		std::cout << make_man_page(cli, "test-net-client");
		return -1;
	}
	 
	std::unique_ptr<IClient> client;
	if (param.raw) {
		client.reset(new UdpClient());
	}
	else {
		client.reset(new SessionClient());
	}

	if (!client->Init(param)) {
		std::cout << "Init client failed!" << std::endl;
		return -1; 
	}

	if (!client->Start()) {
		std::cout << "Start client failed!" << std::endl;
		return -1;
	}
		 
	while (true) {
		jukey::util::Sleep(1000);
	}
	
	return 0; 
}
