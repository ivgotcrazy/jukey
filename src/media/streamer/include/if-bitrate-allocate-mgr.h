#pragma once

#include <inttypes.h>
#include <string>

#include "component.h"


// Component ID and interface ID
#define CID_BITRATE_ALLOCATE_MGR "cid-bitrate-allocate-mgr"
#define IID_BITRATE_ALLOCATE_MGR "iid-bitrate-allocate-mgr"

namespace jukey::stmr
{

//==============================================================================
// 
//==============================================================================
class IBitrateListener
{
public:
	virtual void UpdateBitrate(uint32_t br_kbps) = 0;
};

//==============================================================================
// 
//==============================================================================
class IBitrateAllocateMgr : public base::IUnknown
{
public:
	virtual void UpdateBitrate(const std::string& stream_id, uint32_t br_kbps) = 0;
	virtual void RegsiterListener(const std::string& stream_id, 
		IBitrateListener* listener) = 0;
	virtual void UnregisterListener(const std::string& stream_id,
		IBitrateListener* listener) = 0;
};

}