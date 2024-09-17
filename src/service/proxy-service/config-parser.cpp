#include "config-parser.h"
#include "yaml-cpp/yaml.h"
#include "common/util-net.h"
#include "log.h"
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

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ParseRouteConfig(const YAML::Node& root, std::vector<RouteServiceConfig>& routes)
{
	uint32_t service_type;
	PARSE_CONFIG(root, ARRAY("route-service", "type"), service_type);

	std::string service_name;
	PARSE_CONFIG(root, ARRAY("route-service", "name"), service_name);

	if (!root["route-service"]["entries"]) {
		LOG_ERR("cannot find route-service:entries entry");
		return false;
	}
	NodeVec services = root["route-service"]["entries"].as<NodeVec>();

	for (auto& service : services) {
		RouteServiceConfig entry;

		entry.service_name = service_name;
		entry.service_type = service_type;

		PARSE_CONFIG(service, ARRAY("instance"), entry.instance_id);
		PARSE_CONFIG(service, ARRAY("addr"), entry.service_addr);

		routes.push_back(entry);
	}

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
bool ParsePingConfig(const YAML::Node& root, PingConfig& config)
{
	PARSE_CONFIG(root, ARRAY("service-ping", "interval"), config.interval);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::optional<ProxyServiceConfig> ParseConfig(const std::string& config_file)
{
	ProxyServiceConfig config;

	try {
		YAML::Node root = YAML::LoadFile(config_file);

		if (!ParseServiceConfig(root, config.service_config)) {
			LOG_ERR("parse service config failed");
			return std::nullopt;
		}

		if (!ParseRouteConfig(root, config.route_services)) {
			LOG_ERR("parse routes config failed");
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
	}
	catch (const std::exception& e) {
		LOG_ERR("Parse config file failed, error:{}", e.what());
		return std::nullopt;
	}

	LOG_INF("Parse config file success");

	return config;
}

}