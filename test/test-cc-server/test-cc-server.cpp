#include <iostream>

#include "clipp.h"
#include "session-server.h"
#include "common/util-time.h"

using namespace clipp;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
int main(int argc, char** argv)
{
	std::string addr = "UDP:127.0.0.1:3333";
	bool raw = false;
	uint32_t log_level = 2;

	auto cli = (value("{TCP|UDP]}:IP:Port", addr),
		option("-r", "--raw").set(raw).doc("raw udp/tcp"),
		option("-l", "--log-level") & value("log level", log_level)
		);

	if (!parse(argc, argv, cli)) {
		std::cout << make_man_page(cli, "test-net-server");
		return -1;
	}

	std::unique_ptr<IServer> server(new SessionServer());

	if (!server->Init(addr, log_level)) {
		std::cout << "Init failed!" << std::endl;
		return -1;
	}

	if (!server->Start()) {
		std::cout << "Start failed!" << std::endl;
		return -1;
	}

	while (true) {
		jukey::util::Sleep(1000);
	}

	return 0;
}