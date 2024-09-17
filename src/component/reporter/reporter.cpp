#include "reporter.h"
#include "common/util-common.h"
#include "log.h"

namespace
{

using namespace jukey::com;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void TimerCallback(jukey::base::IUnknown* owner, int64_t data)
{
	IReporter* i = (IReporter*)owner->QueryInterface(IID_REPORTER);
	Reporter* reporter = dynamic_cast<Reporter*>(i);
	reporter->SendReportInfo();
}

}

namespace jukey::com
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Reporter::Reporter(base::IComFactory* factory, const char* owner)
	: base::ProxyUnknown(nullptr)
	, base::ComObjTracer(factory, CID_REPORTER, owner)
	, m_factory(factory)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Reporter::~Reporter()
{
	Stop();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* Reporter::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_REPORTER) == 0) {
		return new Reporter(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* Reporter::NDQueryInterface(const char* riid)
{
	if (0 == strcmp(riid, IID_REPORTER)) {
		return static_cast<IReporter*>(this);
	}
	else {
		return ProxyUnknown::NDQueryInterface(riid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool Reporter::Init(const ReporterParam& param)
{
	if (param.sender == nullptr) {
		LOG_ERR("[{}|{}] invalid sender", param.service_type, param.instance);
		return false;
	}

	if (param.update_interval == 0) {
		LOG_ERR("[{}|{}] invalid update interval", param.service_type, param.instance);
		return false;
	}

	m_report_info.instance = param.instance;
	m_report_info.service_type = param.service_type;
	m_report_info.app = param.app;
	m_report_info.space = param.space;

	m_report_param = param;

	m_timer_mgr = QUERY_TIMER_MGR(m_factory);
	if (!m_timer_mgr) {
		LOG_ERR("[{}|{}] get timer manger failed", param.service_type, param.instance);
		return false;
	}

	LOG_INF("[{}|{}] Init reporter success", param.service_type, param.instance);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void Reporter::Start()
{
	com::TimerParam timer_param;
	timer_param.timeout    = m_report_param.update_interval * 1000;
	timer_param.timer_type = TimerType::TIMER_TYPE_LOOP;
	timer_param.timer_name = "report timer";
	timer_param.timer_func = [this](int64_t) {
		this->SendReportInfo();
	};

	m_report_timer = m_timer_mgr->AllocTimer(timer_param);
	m_timer_mgr->StartTimer(m_report_timer);

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		SendReportInfo();
	}

	m_started = true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void Reporter::Stop()
{
	if (m_report_timer != INVALID_TIMER_ID) {
		m_timer_mgr->StopTimer(m_report_timer);
		m_report_timer = INVALID_TIMER_ID;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Reporter::DepVecIter Reporter::FindDependEntry(const std::string& name)
{
	auto& depends = m_report_info.depends;

	for (auto it = depends.begin(); it != depends.end(); ++it) {
		if (it->name == name) {
			return it;
		}
	}

	return m_report_info.depends.end();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void Reporter::AddReportEntry(const DependEntry& entry)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (FindDependEntry(entry.name) != m_report_info.depends.end()) {
		LOG_ERR("[{}|{}] Report entry exists already, name:{}", 
			m_report_param.service_type, m_report_param.instance, entry.name);
		return;
	}

	m_report_info.depends.push_back(entry);
	
	LOG_INF("[{}|{}] Add depend entry, name:{}, target type:{}, target ID:{}, "
		"state:{}",
		m_report_param.service_type,
		m_report_param.instance, 
		entry.name, 
		entry.target_type, 
		entry.target_id, 
		entry.state);

	if (m_started) {
		SendReportInfo();
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void Reporter::UpdateReportEntry(const std::string& name, uint32_t state)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = FindDependEntry(name);
	if (iter == m_report_info.depends.end()) {
		LOG_ERR("[{}|{}] Cannot find report entry, name:{}", 
			m_report_param.service_type, m_report_param.instance, name);
		return;
	}

	if (iter->state != state) {
		LOG_INF("[{}|{}] Update depend entry, name:{}, target:{}|{}, old state:{}, "
			"new state:{}",
			m_report_param.service_type,
			m_report_param.instance,
			iter->name, 
			iter->target_type, 
			iter->target_id, 
			iter->state, 
			state);

		// Update depend state
		iter->state = state;

		// Send report immediately
		SendReportInfo();
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void Reporter::RemoveReportEntry(const std::string& name)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = FindDependEntry(name);
	if (iter == m_report_info.depends.end()) {
		LOG_WRN("[{}|{}] Cannot find report entry, name:{}", 
			m_report_param.service_type, m_report_param.instance, name);
		return;
	}

	m_report_info.depends.erase(iter);

	LOG_INF("[{}|{}] Remove report entry, name:{}", m_report_param.service_type,
		m_report_param.instance, name);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void Reporter::SendReportInfo()
{
	std::string data = ((json)m_report_info).dump();

	// FIXME: 当模拟丢包时，这个接口会卡住10几秒，最好将发送报告逻辑放到独立线程，以免影响
	// 全局定时器
	//m_report_param.sender->SendReport(data);

	LOG_INF("[{}|{}] Send report data:{}", m_report_param.service_type,
		m_report_param.instance, data);
}

}
