#pragma once

#include <string>
#include <memory>

#include "common-enum.h"
#include "common-error.h"

namespace jukey::stmr
{

//==============================================================================
// Audio and video synchronize handler
//==============================================================================
class ISyncHandler
{
public:
	/**
	 * @brief Update synchronize timestamp
	 */
	virtual void OnSyncUpdate(uint64_t timestamp) = 0;
};

//==============================================================================
// Audio and video synchronize manager
//==============================================================================
class ISyncMgr
{
public:
	/**
	 * @brief Virtual destruction 
	 */
	virtual ~ISyncMgr() {}

	/**
	 * @brief Update synchronize timestamp
	 */
	virtual com::ErrCode UpdateTimestamp(uint64_t sync_id, 
		uint64_t timestamp) = 0;

	/**
	 * @brief Add synchronize handler
	 */
	virtual com::ErrCode AddSyncHandler(uint64_t sync_id,
		ISyncHandler* handler) = 0;

	/**
	 * @brief Remove synchronize handler
	 */
	virtual com::ErrCode RemoveSyncHandler(uint64_t sync_id,
		ISyncHandler* handler) = 0;
};
typedef std::shared_ptr<ISyncMgr> ISyncMgrSP;

}