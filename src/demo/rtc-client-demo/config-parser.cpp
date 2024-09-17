#include <fstream>

#include "config-parser.h"
#include "rapidxml.hpp"
#include "log.h"


namespace jukey::demo
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ConfigParser& ConfigParser::Instance()
{
	static ConfigParser parser;
	return parser;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::optional<std::string>
	ConfigParser::ReadConfigFile(const std::string& config_file)
{
	std::ifstream ifs(config_file);
	if (!ifs) {
		return std::nullopt;
	}

	// get length of file:
	ifs.seekg(0, ifs.end);
	uint32_t length = ifs.tellg();
	ifs.seekg(0, ifs.beg);

	length += 1; // end of '\0'

	// allocate memory:
	char* buffer = new char[length];
	memset(buffer, 0, length);

	// read data as a block:
	ifs.read(buffer, length - 1);
	//if (!ifs) {
	//	int len = ifs.gcount();
	//	return std::nullopt;
	//}
	ifs.close();

	return std::string(buffer);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ConfigParser::Init(const std::string& config_file)
{
	std::optional<std::string> config = ReadConfigFile(config_file);
	if (!config) {
		LOG_ERR("Read config file failed");
		return false;
	}

	rapidxml::xml_document doc;
	doc.parse<0>((char*)config.value().c_str());

	rapidxml::xml_node<char>* root = doc.first_node("root");
	if (!root) {
		LOG_ERR("Get root node failed");
		return false;
	}

	rapidxml::xml_node<>* server_addr = root->first_node("server-addr");
	if (!server_addr) {
		LOG_ERR("Get root:server-addr node failed");
		return false;
	}
	m_demo_config.server_addr = server_addr->value();

	rapidxml::xml_node<>* client_id = root->first_node("client-id");
	if (!client_id) {
		LOG_ERR("Get root:client-id node failed");
		return false;
	}
	m_demo_config.client_id = std::atoi(client_id->value());

	rapidxml::xml_node<>* client_name = root->first_node("client-name");
	if (!client_name) {
		LOG_ERR("Get root:client-name node failed");
		return false;
	}
	m_demo_config.client_name = client_name->value();

	rapidxml::xml_node<>* app_id = root->first_node("app-id");
	if (!app_id) {
		LOG_ERR("Get root:app-id node failed");
		return false;
	}
	m_demo_config.app_id = std::atoi(app_id->value());

	LOG_INF("Parse config file success");

	return true;
}

}