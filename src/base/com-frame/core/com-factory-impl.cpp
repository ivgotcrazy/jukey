#include "com-factory-impl.h"
#include "component.h"
#include "log.h"
#include "common/util-time.h"

#ifdef _WINDOWS
#include "win-dynamic-loader.h"
#endif

#ifdef _LINUX
#include "linux-dynamic-loader.h"
#endif

#define MAX_CID_SIZE 64


namespace jukey::base
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ComFactoryImpl::ComFactoryImpl()
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ComFactoryImpl::~ComFactoryImpl()
{
	// FIXME: coredump, cause by global logger destroy order???
	if (m_timer_mgr) {
		//m_timer_mgr->Stop(); 
		//m_timer_mgr->Release();
		//m_timer_mgr = nullptr;
	}
}

//------------------------------------------------------------------------------
// Singleton
//------------------------------------------------------------------------------
IComFactory* ComFactoryImpl::Instance()
{
	static ComFactoryImpl instance;
	return (&instance);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ComFactoryImpl::InitTimerMgr()
{
	IUnknown* timer_mgr = CreateComponent(CID_TIMER_MGR, "component factory");
	if (!timer_mgr) {
		LOG_ERR("Create timer manager failed!");
		return;
	}

	m_timer_mgr = (com::ITimerMgr*)timer_mgr->QueryInterface(IID_TIMER_MGR);
	if (!m_timer_mgr) {
		LOG_ERR("QueryInterface iid-timer-mgr failed!");
		return;
	}

	m_timer_mgr->Start();

	LOG_INF("Init timer manager");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ComFactoryImpl::Init(const std::string& com_path)
{
	LOG_INF("Init component factory start");

#ifdef _WINDOWS
	IDynamicLoader* loader = new WinDynamicLoader();
#elif _LINUX
	IDynamicLoader* loader = new LinuxDynamicLoader();
#endif

	// Load all lib and get export function address
	IDynamicLoader::EntryVec dynamic_entries;
	loader->Load(com_path, COMPONENT_ENTRY_NAME, &dynamic_entries);

	// Load component by export function address
	for (auto& dynamic_entry : dynamic_entries) {
		LOG_INF("Begin to load component entry from {}", dynamic_entry.lib_name);

		// Get all component objects
		ComEntry* com_entries = nullptr;
		uint32_t com_count = 0;
		dynamic_entry.func(&com_entries, &com_count);

		// None!!!
		if (com_count == 0) {
			LOG_WRN("Found no component object!");
			continue;
		}

		// Tranverse all components
		for (uint32_t i = 0; i < com_count; i++) {
			ComEntry* entry = (com_entries + i);
			if (m_creators.end() != m_creators.find(entry->com_cid)) {
				LOG_WRN("Component {} already exists!", entry->com_cid);
				continue;
			}
			m_creators.insert(std::make_pair(entry->com_cid, entry->com_create));
			LOG_INF("Load one component entry: {}", entry->com_cid);
		}
	}

	InitTimerMgr(); // Inhold timer manager

	return true;
}

//------------------------------------------------------------------------------
// Create component by componet ID, add reference automatically
//------------------------------------------------------------------------------
IUnknown* ComFactoryImpl::CreateComponent(const std::string& cid,
	const std::string& owner)
{
	if (cid.empty()) {
		LOG_ERR("Invalid cid!");
		return nullptr;
	}

	for (auto iter = m_creators.begin(); iter != m_creators.end(); iter++) {
		if (iter->first == cid) {
			IUnknown* unknown = (iter->second)(this, cid.c_str(), owner.c_str());
			if (!unknown) {
				LOG_ERR("Failed to create component {}", cid);
				return nullptr;
			}
			unknown->AddRef();
			LOG_INF("Create component:{} success", cid);
			return unknown;
		}
	}

	LOG_ERR("Cannot find component {}", cid);
	return nullptr;
}

//------------------------------------------------------------------------------
// Get component interface directly without create component
//------------------------------------------------------------------------------
void* ComFactoryImpl::QueryInterface(const std::string& cid,
	const std::string& iid, const std::string& owner)
{
	if (strcmp(cid.c_str(), CID_TIMER_MGR) == 0 && m_timer_mgr) {
		return m_timer_mgr;
	}

	IUnknown* component = CreateComponent(cid, owner);
	if (!component) {
		LOG_ERR("Create component {} failed!", cid);
		return nullptr;
	}
	return component->QueryInterface(iid.c_str());
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ComFactoryImpl::GetComponents(const std::string& iid,
	std::vector<std::string>& cids)
{
	for (auto iter = m_creators.begin(); iter != m_creators.end(); iter++) {
		IUnknown* component = (iter->second)(this, iter->first.c_str(), "unknown");
		if (!component) {
			LOG_ERR("Failed to create component {}", iter->first);
			continue;
		}

		// TODO: inteface should/must derive from IUnknown ?
		IUnknown* intf = (IUnknown*)component->QueryInterface(iid.c_str());
		if (intf) {
			cids.push_back(iter->first);
		}
		component->Release(); // release component
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ComFactoryImpl::StartDumpTimer()
{
	if (m_timer_id != 0) {
		m_timer_mgr->StopTimer(m_timer_id);
		m_timer_mgr->FreeTimer(m_timer_id);
		m_timer_id = 0;
	}

	com::TimerParam timer_param;
	timer_param.timer_type = com::TimerType::TIMER_TYPE_ONCE;
	timer_param.timer_name = "component factory dump timer";
	timer_param.timeout    = 5000; // ms
	timer_param.run_atonce = false;
	timer_param.user_data  = 0;
	timer_param.timer_func = [this](int64_t param) -> void {
		if (m_accu_count > 0) {
			std::lock_guard<std::mutex> lock(m_mutex);
			DumpComObjs();
			m_accu_count = 0;
			m_last_dump = util::Now();
		}
	};

	m_timer_id = m_timer_mgr->AllocTimer(timer_param);
	m_timer_mgr->StartTimer(m_timer_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ComFactoryImpl::DumpComObjs()
{
	for (auto item : m_com_objs) {
		LOG_INF("--- cid:{}, oid:{}, owner:{}", item.second.cid, item.second.oid,
			item.second.owner);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ComFactoryImpl::TryDumpComObjs()
{
	if (!m_timer_mgr) return;

	if (m_last_dump + 1000 * 1000 * 5 > util::Now()) {
		++m_accu_count;
		return;
	}

	DumpComObjs();

	m_accu_count = 0;

	m_last_dump = util::Now();

	StartDumpTimer();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ComFactoryImpl::AddComObj(const ComObj& co)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_com_objs.find(co.oid) != m_com_objs.end()) {
		LOG_ERR("Component object already exists, oid:{}", co.oid);
		return;
	}

	m_com_objs.insert(std::make_pair(co.oid, co));

	LOG_INF("Add component object, owner:{}, cid:{}, oid:{}", co.owner, co.cid,
		co.oid);

	TryDumpComObjs();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ComFactoryImpl::RemoveComObj(const std::string& oid)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_com_objs.find(oid);
	if (iter == m_com_objs.end()) {
		LOG_ERR("Cannot find component object, oid:{}", oid);
		return;
	}

	LOG_INF("Remove component object, cid:{}, oid:{}, owner:{}", 
		iter->second.cid, oid, iter->second.owner);

	m_com_objs.erase(iter);

	TryDumpComObjs();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::vector<ComObj> ComFactoryImpl::GetComObjList(const std::string& cid)
{
	std::vector<ComObj> objs;

	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto item : m_com_objs) {
		if (cid.empty() || item.second.cid == cid) {
			objs.push_back(item.second);
		}
	}

	return objs;
}

}