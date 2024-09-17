#include "config-parser.h"
#include "log.h"
#include "common/util-net.h"
#include "yaml-cpp/yaml.h"
#include "service-define.h"

using namespace jukey::util;
using namespace jukey::com;

namespace jukey::srv
{

PARSE_CONFIG_ENTRY_IMPL

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ParseServiceConfig(const YAML::Node& root, ServiceConfig& config)
{
	PARSE_CONFIG(root, ARRAY("service", "type"), config.service_type);
	PARSE_CONFIG(root, ARRAY("service", "name"), config.service_name);
	PARSE_CONFIG(root, ARRAY("service", "instance"), config.instance_id);
	PARSE_CONFIG(root, ARRAY("service", "exchange"), config.exchange);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ParseRabbitMqConfig(const YAML::Node& root, MqConfig& config)
{
	PARSE_CONFIG(root, ARRAY("rabbitmq", "addr"), config.addr);
	PARSE_CONFIG(root, ARRAY("rabbitmq", "user"), config.user);
	PARSE_CONFIG(root, ARRAY("rabbitmq", "pwd"), config.pwd);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ParseReportConfig(const YAML::Node& root, ReportConfig& config)
{
	PARSE_CONFIG(root, ARRAY("report", "host"), config.host);
	PARSE_CONFIG(root, ARRAY("report", "port"), config.port);
	PARSE_CONFIG(root, ARRAY("report", "path"), config.path);
	PARSE_CONFIG(root, ARRAY("report", "interval"), config.interval);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ParseRouteConfig(const YAML::Node& root, RouteConfig& config)
{
	PARSE_CONFIG(root, ARRAY("route-service", "type"), config.service_type);
	PARSE_CONFIG(root, ARRAY("route-service", "exchange"), config.exchange);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ParseUserConfig(const YAML::Node& root, UserConfig& config)
{
	PARSE_CONFIG(root, ARRAY("user-service", "notify-exchange"), 
		config.notify_exchange);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ParsePingConfig(const YAML::Node& root, PingConfig& config)
{
	PARSE_CONFIG(root, ARRAY("service-ping", "exchange"), config.exchange);
	PARSE_CONFIG(root, ARRAY("service-ping", "interval"), config.interval);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ParseStreamConfig(const YAML::Node& root, StreamConfig& config)
{
	PARSE_CONFIG(root, ARRAY("stream-service", "type"), config.service_type);
	PARSE_CONFIG(root, ARRAY("stream-service", "name"), config.service_name);
	PARSE_CONFIG(root, ARRAY("stream-service", "exchange"), config.exchange);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::optional<GroupServiceConfig> ParseConfig(const std::string& config_file)
{
	GroupServiceConfig config;
	try {
		YAML::Node root = YAML::LoadFile(config_file);

		if (!ParseServiceConfig(root, config.service_config)) {
			LOG_ERR("parse service config failed");
			return std::nullopt;
		}

		if (!ParseRabbitMqConfig(root, config.mq_config)) {
			LOG_ERR("parse rabbitmq config failed");
			return std::nullopt;
		}

		if (!ParseReportConfig(root, config.report_config)) {
			LOG_ERR("parse report config failed");
			return std::nullopt;
		}

		if (!ParseRouteConfig(root, config.route_config)) {
			LOG_ERR("parse route service config failed");
			return std::nullopt;
		}

		if (!ParseUserConfig(root, config.user_config)) {
			LOG_ERR("parse user service config failed");
			return std::nullopt;
		}

		if (!ParseStreamConfig(root, config.stream_config)) {
			LOG_ERR("parse stream service config failed");
			return std::nullopt;
		}

		if (!ParsePingConfig(root, config.ping_config)) {
			LOG_ERR("parse service ping config failed");
			return std::nullopt;
		}
	}
	catch (const std::exception& e) {
		LOG_ERR("Error:{}", e.what());
		return std::nullopt;
	}

	LOG_INF("Parse config file success");

	return config;
}

}