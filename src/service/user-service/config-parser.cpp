#include "config-parser.h"
#include "log.h"
#include "common/util-net.h"
#include "yaml-cpp/yaml.h"
#include "service-define.h"


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
	PARSE_CONFIG(root, ARRAY("service", "notify-exchange"), config.notify_exchange);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ParseMqConfig(const YAML::Node& root, MqConfig& config)
{
	PARSE_CONFIG(root, ARRAY("rabbitmq", "addr"), config.addr);
	PARSE_CONFIG(root, ARRAY("rabbitmq", "user"), config.user);
	PARSE_CONFIG(root, ARRAY("rabbitmq", "pwd"), config.pwd);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ParseRouteConfig(const YAML::Node& root, RouteConfig& config)
{
	PARSE_CONFIG(root, ARRAY("route-service", "exchange"), config.exchange);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ParseTerminalConfig(const YAML::Node& root, TerminalConfig& config)
{
	PARSE_CONFIG(root, ARRAY("terminal-service", "notify-exchange"), 
		config.notify_exchange);

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
std::optional<UserServiceConfig> ParseConfig(const std::string& config_file)
{
	UserServiceConfig config;
	try {
		YAML::Node root = YAML::LoadFile(config_file);

		if (!ParseServiceConfig(root, config.service_config)) {
			LOG_ERR("parse service config failed");
			return std::nullopt;
		}

		if (!ParseMqConfig(root, config.mq_config)) {
			LOG_ERR("parse mq config failed");
			return std::nullopt;
		}

		if (!ParseRouteConfig(root, config.route_config)) {
			LOG_ERR("parse route config failed");
			return std::nullopt;
		}

		if (!ParseTerminalConfig(root, config.terminal_config)) {
			LOG_ERR("parse terminal config failed");
			return std::nullopt;
		}

		if (!ParseReportConfig(root, config.report_config)) {
			LOG_ERR("parse report config failed");
			return std::nullopt;
		}
	}
	catch (const std::exception& e) {
		LOG_ERR("Parse config file failed, error:{}", e.what());
		return std::nullopt;
	}

	LOG_INF("Parse config file success");

	return config;
}

}