#pragma once

#include <set>
#include <optional>

#include "common-struct.h"

// yaml-cpp warning
#pragma warning(disable:4251)
#pragma warning(disable:4275)

namespace jukey::srv
{

//==============================================================================
// 
//==============================================================================
struct ServiceConfig
{
	com::Address listen_addr;
	uint32_t service_type;
	std::string service_name;
	std::string instance_id;
	std::string exchange;
};

//==============================================================================
// 
//==============================================================================
struct RouteEntry
{
	uint32_t service_type = 0;
	std::string service_name;
	std::string exchange;
	std::set<uint32_t> messages;
};

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
struct PingConfig
{
	std::string exchange;
	uint32_t interval = 10;
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
struct RouteServiceConfig
{
	ServiceConfig service_config;
	MqConfig mq_config;
	ReportConfig report_config;
	PingConfig ping_config;
	std::vector<RouteEntry> routes;
};

//
// Parse configuration
//
std::optional<RouteServiceConfig> ParseConfig(const std::string& config_file);

}