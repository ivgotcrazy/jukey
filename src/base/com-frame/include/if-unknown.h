#pragma once

#include <inttypes.h>

namespace jukey::base
{

// BF: Base Frame
#define BF_IID_IUNKNOWN "BF_IID_IUNKNOWN"

//==============================================================================
// Simulate windows COM interface
//==============================================================================
struct IUnknown
{
	virtual void* QueryInterface(const char* riid) = 0;
	virtual uint32_t AddRef() = 0;
	virtual uint32_t Release() = 0;
};

}