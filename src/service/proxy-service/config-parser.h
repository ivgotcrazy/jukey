#pragma once

#include <optional>

#include "common-struct.h"
#include "proxy-common.h"

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
	uint32_t service_type = 0;
	std::string service_name;
	std::string instance_id;
};

//==============================================================================
// 
//==============================================================================
struct ReportConfig
{
	std::string host;
	std::string path;
	uint16_t port = 0;
	uint32_t interval = 10;
};

//==============================================================================
// 
//==============================================================================
struct RouteServiceConfig
{
	uint32_t service_type = 0;
	std::string service_name;
	std::string instance_id;
	com::Address service_addr;
};

//==============================================================================
// 
//==============================================================================
struct PingConfig
{
	uint32_t interval = 10;
};

//==============================================================================
// 
//==============================================================================
struct ProxyServiceConfig
{
	ServiceConfig service_config;
	ReportConfig report_config;
	PingConfig ping_config;
	std::vector<RouteServiceConfig> route_services;
};

//
// Parse configure
//
std::optional<ProxyServiceConfig> ParseConfig(const std::string& config_file);

}