#pragma once

#include "if-rtt-filter.h"
#include "net-public.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class RttFilter : public IRttFilter
{
public:
	RttFilter(SessionId local_sid);

	// IRttFilter
	virtual uint32_t UpdateRtt(uint32_t rtt) override;
	virtual uint32_t GetRto() override;

private:
	SessionId m_local_sid = 0;
	uint32_t m_rtt_var = 0;
	uint32_t m_srtt = 0;
	uint32_t m_rto = 0;
};

}