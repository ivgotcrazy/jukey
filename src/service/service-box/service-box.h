#pragma once

#include <vector>
#include <filesystem>

#include "if-service.h"
#include "if-service-box.h"
#include "if-session-mgr.h"
#include "com-factory.h"

// yaml-cpp warning
#pragma warning( disable: 4251 )
#pragma warning( disable: 4275 )

namespace jukey::srv
{

//==============================================================================
// 
//==============================================================================
class ServiceBox : public IServiceBox
{
public:
	// IServiceBox
	virtual bool Init(const std::string& config_file) override;
	virtual void Run() override;

private:
	struct ServiceConfigEntry
	{
		std::string name;
		std::string cid;
		std::string config;
	};
	typedef std::vector<ServiceConfigEntry> ServiceConfigEntryVec;

	struct SrvBoxConfig
	{
		std::string com_path;
	uint32_t load_config_interval = 1;
		ServiceConfigEntryVec services;
	};

	struct ServiceItem
	{
	std::string config_file;
	std::filesystem::file_time_type last_write;
	IService* service = nullptr;
	};

private:
	bool ParseConfig(const std::string& config_file, SrvBoxConfig& config);
	bool LoadConfig(const std::string& config_file);
	void MonitorConfigFiles();

private:
	jukey::base::IComFactory* m_factory = nullptr;
	jukey::net::ISessionMgr* m_sess_mgr = nullptr;

	SrvBoxConfig m_config;
	std::vector<ServiceItem> m_services;
};

}

