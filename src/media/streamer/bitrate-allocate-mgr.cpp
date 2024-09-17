#include "bitrate-allocate-mgr.h"
#include "log.h"

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
BitrateAllocateMgr::BitrateAllocateMgr(base::IComFactory* factory, const char* owner)
	: ProxyUnknown(nullptr)
	, ComObjTracer(factory, CID_BITRATE_ALLOCATE_MGR, owner)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
BitrateAllocateMgr::~BitrateAllocateMgr()
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* BitrateAllocateMgr::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_BITRATE_ALLOCATE_MGR) == 0) {
		return new BitrateAllocateMgr(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* BitrateAllocateMgr::NDQueryInterface(const char* riid)
{
	if (0 == strcmp(riid, IID_BITRATE_ALLOCATE_MGR)) {
		return static_cast<IBitrateAllocateMgr*>(this);
	}
	else {
		return ProxyUnknown::NDQueryInterface(riid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void BitrateAllocateMgr::UpdateBitrate(const std::string& stream_id,
	uint32_t br_kbps)
{
	LOG_INF("Update bitrate, stream:{}, br:{}", stream_id, br_kbps);

	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_listeners.find(stream_id);
	if (iter == m_listeners.end()) {
		LOG_ERR("Cannot find stream:{}", stream_id);
		return;
	}

	iter->second->UpdateBitrate(br_kbps);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void BitrateAllocateMgr::RegsiterListener(const std::string& stream_id,
	IBitrateListener* listener)
{
	LOG_INF("Register listener, stream:{}", stream_id);

	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_listeners.find(stream_id);
	if (iter != m_listeners.end()) {
		LOG_ERR("Stream listener exists, remove it first!");
		m_listeners.erase(iter);
	}

	m_listeners.insert(std::make_pair(stream_id, listener));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void BitrateAllocateMgr::UnregisterListener(const std::string& stream_id,
	IBitrateListener* listener)
{
	LOG_INF("Unregister listener, stream:{}", stream_id);

	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_listeners.find(stream_id);
	if (iter == m_listeners.end()) {
		LOG_ERR("Cannot find listener!");
		return;
	}

	m_listeners.erase(iter);
}

}