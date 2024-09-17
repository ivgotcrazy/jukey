#include "config-parser.h"
#include "log.h"
#include "common/util-net.h"
#include "yaml-cpp/yaml.h"
#include "service-define.h"


typedef std::vector<YAML::Node> NodeVec;

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
	PARSE_CONFIG(root, ARRAY("service", "addr"), config.listen_addr);
	PARSE_CONFIG(root, ARRAY("service", "exchange"), config.exchange);

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
bool ParsePingConfig(const YAML::Node& root, PingConfig& config)
{
	PARSE_CONFIG(root, ARRAY("service-ping", "exchange"), config.exchange);

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
bool ParseRouteConfig(const YAML::Node& root,
	std::vector<RouteEntry>& routes)
{
	if (!root["routes"]) {
		LOG_ERR("cannot find routes config");
		return false;
	}
	NodeVec services = root["routes"].as<NodeVec>();

	for (auto& service : services) {
		RouteEntry route;

		PARSE_CONFIG(service, ARRAY("type"), route.service_type);
		PARSE_CONFIG(service, ARRAY("name"), route.service_name);
		PARSE_CONFIG(service, ARRAY("exchange"), route.exchange);

		if (!service["messages"]) {
			LOG_ERR("cannot find routes:messages entry");
			return false;
		}
		NodeVec messages = service["messages"].as<NodeVec>();
		for (auto& msg : messages) {
			route.messages.insert(msg.as<uint32_t>());
		}

		routes.push_back(route);
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::optional<RouteServiceConfig> ParseConfig(const std::string& config_file)
{
	RouteServiceConfig config;

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

		if (!ParseReportConfig(root, config.report_config)) {
			LOG_ERR("parse report config failed");
			return std::nullopt;
		}

		if (!ParsePingConfig(root, config.ping_config)) {
			LOG_ERR("parse ping config failed");
			return std::nullopt;
		}

		if (!ParseRouteConfig(root, config.routes)) {
			LOG_ERR("parse routes config failed");
			return std::nullopt;
		}
	}
	catch (const YAML::KeyNotFound& e) {
		LOG_ERR("Parse config file failed, error:{}", e.what());
		return std::nullopt;
	}
	catch (const YAML::InvalidNode& e) {
		LOG_ERR("Parse config file failed, error:{}", e.what());
		return std::nullopt;
	}
	catch (const YAML::ParserException& e) {
		LOG_ERR("Parse config file failed, error:{}", e.what());
		return std::nullopt;
	}
	catch (const YAML::Exception& e) {
		LOG_ERR("Parse config file failed, error:{}", e.what());
		return std::nullopt;
	}

	LOG_INF("Parse config file success");

	return config;
}

}