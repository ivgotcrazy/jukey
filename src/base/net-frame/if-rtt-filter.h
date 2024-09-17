#pragma once

#include <inttypes.h>
#include <memory>

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class IRttFilter
{
public:
	virtual uint32_t UpdateRtt(uint32_t rtt) = 0;

	virtual uint32_t GetRto() = 0;
};
typedef std::shared_ptr<IRttFilter> IRttFilterSP;

}