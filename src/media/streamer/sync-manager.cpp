#include "sync-manager.h"
#include "log.h"

using namespace jukey::com;

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ISyncMgr& SyncManager::Instance()
{
	static SyncManager sync_mgr;
	return sync_mgr;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SyncManager::AddSynchronizer(uint64_t sync_id)
{
	if (m_synchronizers.find(sync_id) != m_synchronizers.end()) {
		LOG_ERR("Synchronizer:{} already exists!", sync_id);
		return false;
	}

	m_synchronizers.insert(std::make_pair(sync_id, std::set<ISyncHandler*>()));

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SyncManager::RemoveSynchronizer(uint64_t sync_id)
{
	auto size = m_synchronizers.erase(sync_id);
	if (size == 0) {
		LOG_ERR("Erase synchronizer:{} failed!", sync_id);
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode SyncManager::UpdateTimestamp(uint64_t sync_id, uint64_t timestamp)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_synchronizers.find(sync_id) == m_synchronizers.end()) {
		if (!AddSynchronizer(sync_id)) {
			LOG_INF("Add synchronizer:{} failed!", sync_id);
			return ERR_CODE_FAILED;
		}
	}

	auto iter = m_synchronizers.find(sync_id);
	if (iter == m_synchronizers.end()) {
		LOG_INF("Cannot find synchronizer:{}", sync_id);
		return ERR_CODE_FAILED;
	}

	for (auto item : iter->second) {
		item->OnSyncUpdate(timestamp);
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode SyncManager::AddSyncHandler(uint64_t sync_id, ISyncHandler* handler)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_synchronizers.find(sync_id) == m_synchronizers.end()) {
		if (!AddSynchronizer(sync_id)) {
			LOG_INF("Add synchronizer:{} failed!", sync_id);
			return ERR_CODE_FAILED;
		}
	}

	auto iter = m_synchronizers.find(sync_id);
	if (iter == m_synchronizers.end()) {
		LOG_INF("Cannot find synchronizer:{}", sync_id);
		return ERR_CODE_FAILED;
	}

	auto result = iter->second.insert(handler);
	if (!result.second) {
		LOG_ERR("Add sync handler to {} failed!", sync_id);
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode SyncManager::RemoveSyncHandler(uint64_t sync_id, ISyncHandler* handler)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_synchronizers.find(sync_id);
	if (iter == m_synchronizers.end()) {
		LOG_ERR("Cannot find synchronizer:{}", sync_id);
		return ERR_CODE_FAILED;
	}

	auto size = iter->second.erase(handler);
	if (size == 0) {
		LOG_ERR("Remove sync handler from {} failed!", sync_id);
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

}