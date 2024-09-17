#pragma once

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
	com::Address listen_addr;
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
struct TransportServiceConfig
{
	ServiceConfig service_config;
	MqConfig mq_config;
	ReportConfig report_config;
};

//
// Parse configuration
//
std::optional<TransportServiceConfig> ParseConfig(const std::string& config_file);

}