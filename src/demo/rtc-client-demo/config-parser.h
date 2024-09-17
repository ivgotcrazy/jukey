#pragma once

#include <string>
#include <optional>

namespace jukey::demo
{

//==============================================================================
// 
//==============================================================================
struct DemoConfig
{
	uint32_t app_id = 0;
	uint32_t client_id = 0;
	std::string server_addr;
	std::string client_name;
};

//==============================================================================
// 
//==============================================================================
class ConfigParser
{
public:
	static ConfigParser& Instance();

	bool Init(const std::string& config_file);

	const DemoConfig& Config() { return m_demo_config; }

private:
	ConfigParser() {}
	ConfigParser(const ConfigParser&) = delete;
	ConfigParser& operator=(const ConfigParser&) = delete;

	std::optional<std::string> ReadConfigFile(const std::string& config_file);

private:
	DemoConfig m_demo_config;
};

}