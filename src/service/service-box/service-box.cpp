#include <filesystem>
#include <fstream>
#include <cerrno>

#include "service-box.h"
#include "yaml-cpp/yaml.h"
#include "common/util-time.h"
#include "log.h"
#include "md5.h"


using namespace jukey::net;
using namespace jukey::util;

namespace
{

using namespace jukey::srv;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ReadFileData(const std::string& file, std::string& data) 
{
	std::ifstream file_stream(file, std::ios::binary);
	if (!file_stream.is_open()) {
		LOG_ERR("open file:{} failed", file);
		return false;
	}

	file_stream.seekg(0, file_stream.end);
	std::streampos length = file_stream.tellg();
	file_stream.seekg(0, file_stream.beg);
	data.resize(length);

	LOG_INF("get file total length:{}", length);

	file_stream.read(&data[0], length);
	if (!file_stream) {
		char err_msg[256];
		strerror_s(err_msg, sizeof(err_msg), errno);
		LOG_ERR("read failed, length:{}, error:{}", file_stream.gcount(), err_msg);
		file_stream.close();
		return false;
	}

	file_stream.close();
	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool AppendDigestLine(const std::string& file, std::string& data) 
{
	std::ofstream outfile(file, std::ios_base::app);
	if (!outfile.is_open()) {
		return false;
	}

	std::ifstream infile(file, std::ios::binary | std::ios::ate);
	if (infile.tellg() > 0) {
		outfile << std::endl;
	}
	infile.close();

	outfile << data;
	outfile.close();
	
	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void UpdateDigestFile(const std::string& config_file)
{
	// read config data
	std::string config_data;
	if (!ReadFileData(config_file, config_data)) {
		LOG_ERR("read config file:{} data failed", config_file);
		return;
	}

	std::string md5 = MD5(config_data).toStr();

	std::string digest_line = md5 + "," + NowStr();

	if (!AppendDigestLine(config_file + ".digest", digest_line)) {
		LOG_ERR("write digest file data failed")
	}

	LOG_INF("update digest, config file:{}, config length:{}, digest:{}",
		config_file, config_data.length(), md5);
}

}

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ServiceBox::ParseConfig(const std::string& config_file, SrvBoxConfig& config)
{
	try {
		YAML::Node root = YAML::LoadFile(config_file);

		config.com_path = root["component-path"].as<std::string>();

		config.load_config_interval = root["load-config-interval"].as<uint32_t>();

		std::vector<YAML::Node> services = root["services"].as<std::vector<YAML::Node>>();
		for (const auto& service : services) {
			ServiceConfigEntry entry;
			entry.name = service["name"].as<std::string>();
			entry.cid = service["cid"].as<std::string>();
			entry.config = service["config"].as<std::string>();

			config.services.push_back(entry);
		}
	}
	catch (const std::exception& e) {
		LOG_ERR("Error:{}", e.what());
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ServiceBox::LoadConfig(const std::string& config_file)
{
	if (!ParseConfig(config_file, m_config)) {
		LOG_ERR("Parse config:{} failed!", config_file);
		return false;
	}

	for (auto service : m_config.services) {
		if (!std::filesystem::exists(service.config)) {
			LOG_ERR("Cannot find config file:{}", service.config);
			return false;
		}
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ServiceBox::Init(const std::string& config_file)
{
	while (!LoadConfig(config_file)) {
		LOG_INF("waiting for service configure files...");
		std::this_thread::sleep_for(
			std::chrono::seconds(m_config.load_config_interval));
	}

	//
	// Initialize component factory
	//
	m_factory = GetComFactory();
	if (!m_factory) {
		LOG_ERR("Get component factory failed!");
		return false;
	}

	if (!m_factory->Init(m_config.com_path.c_str())) {
		LOG_ERR("Init component factory failed!");
		return false;
	}

	//
	// Initialize session manager
	//
	m_sess_mgr = (ISessionMgr*)QI(CID_SESSION_MGR, IID_SESSION_MGR, 
		"service box");
	if (!m_sess_mgr) {
		LOG_ERR("Create session manager failed!");
		return false;
	}

	SessionMgrParam param;
	param.ka_interval = 5;
	param.thread_count = 4;
	if (com::ErrCode::ERR_CODE_OK != m_sess_mgr->Init(param)) {
		LOG_ERR("Initialize session manager failed!");
		return false;
	}

	//
	// Create and initialize services 
	//
	for (auto item : m_config.services) {
		IService* service = (IService*)m_factory->CreateComponent(item.cid.c_str(), 
			"service box");
		if (service) {
			if (service->Init(m_sess_mgr, item.config)) {
				LOG_INF("Initialize service:{} success", item.name);
				m_services.push_back({
					item.config,
					std::filesystem::last_write_time(item.config),
					service });
				UpdateDigestFile(item.config);
			}
			else {
				LOG_ERR("Initialize service:{} failed!", item.name);
				return false;
			}
		}
		else {
			LOG_ERR("Create component:{} failed!", item.cid);
			return false;
		}
	}

	LOG_INF("Initialize service-box success");

	return true;
}

//------------------------------------------------------------------------------
//	 
//------------------------------------------------------------------------------
void ServiceBox::Run()
{
	for (auto item : m_services) {
		item.service->Start();
	}

	LOG_INF("Service box running");

	for (;;) {
		MonitorConfigFiles();
		util::Sleep(1000000);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServiceBox::MonitorConfigFiles()
{
	for (auto it = m_services.begin(); it != m_services.end(); ) {
		try {
			std::filesystem::path config_path(it->config_file);
			if (std::filesystem::exists(config_path)) {
				auto new_write_time = std::filesystem::last_write_time(config_path);
				if (new_write_time != it->last_write) {
					it->service->Reload();
					it->last_write = new_write_time;
					UpdateDigestFile(it->config_file);
				}
				it++;
			}
		}
		catch (std::exception& e) {
			LOG_ERR("{}", e.what());
		}
	}
}

}