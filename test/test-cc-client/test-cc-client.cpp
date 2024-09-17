#include <iostream>

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
	auto cli = (value("{TCP|UDP]}:IP:Port", param.addr),
		option("-l", "") & value("log level", param.log_level)
		);

	if (!parse(argc, argv, cli)) {
		std::cout << make_man_page(cli, "test-net-client");
		return -1;
	}

	std::unique_ptr<IClient> client(new SessionClient());

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
