#pragma once

#include <optional>

#include "common-struct.h"

// yaml-cpp warning
#pragma warning(disable: 4251)
#pragma warning(disable: 4275)

namespace jukey::srv
{

//==============================================================================
// 
//==============================================================================
struct MqConfig
{
	com::Address addr;
	std::string user;
	std::string pwd;
};

//==============================================================================
// 
//==============================================================================
struct ServiceConfig
{
	uint32_t service_type = 0;
	std::string service_name;
	std::string instance_id;
	std::string exchange;
};

//==============================================================================
// 
//==============================================================================
struct RouteConfig
{
	uint32_t service_type = 0;
	std::string exchange;
};

//==============================================================================
// 
//==============================================================================
struct UserConfig
{
	std::string notify_exchange;
};

//==============================================================================
// 
//==============================================================================
struct PingConfig
{
	std::string exchange;
	uint32_t interval = 10;
};

//==============================================================================
// 
//==============================================================================
struct StreamConfig
{
	std::string service_name;
	uint32_t service_type = 0;
	std::string exchange;
};

//==============================================================================
// 
//==============================================================================
struct ReportConfig
{
	std::string host;
	std::string path;
	uint16_t port = 0;
	uint32_t interval = 30;
};

//==============================================================================
// 
//==============================================================================
struct GroupServiceConfig
{
	ServiceConfig service_config;
	MqConfig mq_config;
	RouteConfig route_config;
	UserConfig user_config;
	StreamConfig stream_config;
	ReportConfig report_config;
	PingConfig ping_config;
};

//==============================================================================
// 
//==============================================================================
std::optional<GroupServiceConfig> ParseConfig(const std::string& config_file);

}