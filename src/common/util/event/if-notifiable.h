#pragma once

#include <inttypes.h>

namespace jukey::util
{

//==============================================================================
// 
//==============================================================================
class INotifiable
{
public:
	virtual ~INotifiable() {}

	//
	// Trigger designated event type
	//
	virtual void Notify(uint32_t event_type) = 0;
};

}