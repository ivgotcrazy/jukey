#pragma once

#include <inttypes.h>
#include <memory>
#include <functional>

#include "component.h"


namespace jukey::com
{

#define CID_TIMER_MGR "cid-timer-mgr"
#define IID_TIMER_MGR "iid-timer-mgr"

#define INVALID_TIMER_ID 0

//==============================================================================
// Timer ID data type
//==============================================================================
typedef uint64_t TimerId;

//==============================================================================
// Timer type
//==============================================================================
enum class TimerType
{
	TIMER_TYPE_ONCE, // Trigger once
	TIMER_TYPE_LOOP  // Trigger looply
};

//==============================================================================
// Timer parameters
//==============================================================================
typedef std::function<void(int64_t)> TimerFunc;

struct TimerParam
{
	TimerType   timer_type = TimerType::TIMER_TYPE_LOOP;
	std::string timer_name;
	bool        run_atonce = false;
	TimerFunc   timer_func;
	uint32_t    timeout = 1000; // ms
	int64_t     user_data = 0;
};

//==============================================================================
// Timer manager
//==============================================================================
class ITimerMgr : public base::IUnknown
{
public:
	//
	// Start timer manager
	//
	virtual void Start() = 0;

	//
	// Stop timer manager
	//
	virtual void Stop() = 0;

	//
	// Allocate a timer
	// 
	virtual TimerId AllocTimer(const TimerParam& param) = 0;

	//
	// Free a timer
	//
	virtual void FreeTimer(TimerId timer_id) = 0;

	//
	// Start a timer
	// 
	virtual void StartTimer(TimerId timer_id) = 0;

	//
	// Stop a timer
	//
	virtual void StopTimer(TimerId timer_id) = 0;

	//
	// Update timeout of timer which is in the idle list
	//
	virtual bool UpdateTimeout(TimerId timer_id, uint32_t timeout_ms) = 0;
};

}
