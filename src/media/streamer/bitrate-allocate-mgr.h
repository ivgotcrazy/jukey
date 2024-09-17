#pragma once

#include <map>

#include "if-bitrate-allocate-mgr.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"

namespace jukey::stmr
{

//==============================================================================
// 
//==============================================================================
class BitrateAllocateMgr 
	: public IBitrateAllocateMgr
	, public base::ProxyUnknown
	, public base::ComObjTracer
{
public:
	BitrateAllocateMgr(base::IComFactory* factory, const char* owner);
	~BitrateAllocateMgr();

	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IBitrateAllocateMgr
	virtual void UpdateBitrate(const std::string& stream_id, 
		uint32_t br_kbps) override;
	virtual void RegsiterListener(const std::string& stream_id,
		IBitrateListener* listener) override;
	virtual void UnregisterListener(const std::string& stream_id,
		IBitrateListener* listener) override;

private:
	std::map<std::string, IBitrateListener*> m_listeners;
	std::mutex m_mutex;
};

}