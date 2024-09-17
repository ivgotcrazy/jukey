#pragma once

#include <map>
#include <set>
#include <mutex>

#include "if-sync-mgr.h"

namespace jukey::stmr
{

//==============================================================================
// 
//==============================================================================
class SyncManager : public ISyncMgr
{
public:
	static ISyncMgr& Instance();

	// ISyncMgr
	virtual com::ErrCode UpdateTimestamp(uint64_t sync_id,
		uint64_t timestamp) override;
	virtual com::ErrCode AddSyncHandler(uint64_t sync_id,
		ISyncHandler* handler) override;
	virtual com::ErrCode RemoveSyncHandler(uint64_t sync_id,
		ISyncHandler* handler) override;

private:
	bool AddSynchronizer(uint64_t sync_id);
	bool RemoveSynchronizer(uint64_t sync_id);

private:
	// TODO: clear idle synchronizers
	std::map<uint64_t, std::set<ISyncHandler*>> m_synchronizers;
	std::mutex m_mutex;
};

}