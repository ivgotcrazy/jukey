#pragma once

#include <inttypes.h>
#include <memory>

namespace jukey::util
{
 
#define WAIT_INFINITE 0xFFFFFFFFFFFFFFFF

//==============================================================================
// 
//==============================================================================
class IEvent
{
public:
	virtual ~IEvent() {}

	//
	// Wait for event to be triggered
	//
	virtual void Wait(uint64_t us) = 0;

	//
	// Trigger event
	//
	virtual void Trigger() = 0;

	//
	// Reset event
	//
	virtual void Reset() = 0;
};
typedef std::shared_ptr<IEvent> IEventSP;

}