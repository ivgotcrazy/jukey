// service-box.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <ostream>

#include "clipp.h"
#include "service-box.h"
#include "log.h"

using namespace clipp;
using namespace jukey::srv;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
int main(int argc, char** argv)
{
	//
	// Command line process 
	//
	std::string config_file = "service-box.yaml";
	auto cli = (option("-c", "--config") & value("config file", config_file));
	if (!parse(argc, argv, cli)) {
		std::ostringstream oss;
		oss << make_man_page(cli, argv[0]);
		LOG_ERR("\n{}", oss.str());
		return -1;
	}

	LOG_INF("Parse command line success");

	//
	// Main loop
	//
	jukey::srv::ServiceBox service_box;
	if (!service_box.Init(config_file)) {
		LOG_ERR("Init service box failed!");
		return -1;
	}
	else {
		service_box.Run();
	}

	LOG_INF("Exit");

	return 0;
}
