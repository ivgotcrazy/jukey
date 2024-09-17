#pragma once

#include <map>
#include <memory>
#include <mutex>

#include "include/if-reporter.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "report-common.h"
#include "if-timer-mgr.h"

namespace jukey::com
{

//==============================================================================
// 
//==============================================================================
class Reporter 
	: public base::ProxyUnknown
	, public base::ComObjTracer
	, public IReporter
{
public:
	Reporter(base::IComFactory* factory, const char* owner);
	~Reporter();

	// ProxyUnknown
	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IReporter
	virtual bool Init(const ReporterParam& param) override;
	virtual void Start() override;
	virtual void Stop() override;
	virtual void AddReportEntry(const DependEntry& entry) override;
	virtual void UpdateReportEntry(const std::string& name, uint32_t state) override;
	virtual void RemoveReportEntry(const std::string& name) override;

	void SendReportInfo();

private:
	typedef std::vector<DependEntry>::iterator DepVecIter;

private:
	DepVecIter FindDependEntry(const std::string& name);

private:
	std::mutex m_mutex;
	base::IComFactory* m_factory = nullptr;
	com::ITimerMgr* m_timer_mgr = nullptr;
	ReportInfo m_report_info;
	ReporterParam m_report_param;
	com::TimerId m_report_timer = INVALID_TIMER_ID;
	bool m_started = false;
};

}