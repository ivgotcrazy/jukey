#pragma once

#include <string>
#include <memory>

#include "common-define.h"
#include "component.h"

namespace jukey::com
{

#define CID_REPORTER "cid-reporter"
#define IID_REPORTER "iid-reporter"

//==============================================================================
// 
//==============================================================================
class IReportSender
{
public:
	virtual ~IReportSender() {}

	virtual void SendReport(const std::string& data) = 0;
};
typedef std::shared_ptr<IReportSender> IReportSenderSP;

//==============================================================================
// 
//==============================================================================
struct DependEntry
{
	std::string name;
	std::string desc;
	std::string protocol;
	std::string target_type;
	std::string target_id;
	uint32_t state = 0;
};

//==============================================================================
// 
//==============================================================================
struct ReporterParam
{
	std::string app;
	std::string space;
	std::string instance;
	std::string service_type;
	uint32_t update_interval = 5; // in second
	IReportSenderSP sender;
};

//==============================================================================
// 
//==============================================================================
class IReporter : public base::IUnknown
{
public:
	//
	// Initialize
	//
	virtual bool Init(const ReporterParam& param) = 0;

	//
	// Start
	//
	virtual void Start() = 0;

	//
	// Stop
	//
	virtual void Stop() = 0;

	//
	// Add report entry
	//
	virtual void AddReportEntry(const DependEntry& entry) = 0;

	//
	// Update report entry that added
	//
	virtual void UpdateReportEntry(const std::string& name, uint32_t state) = 0;

	//
	// Remove report entry
	//
	virtual void RemoveReportEntry(const std::string& name) = 0;
};

}