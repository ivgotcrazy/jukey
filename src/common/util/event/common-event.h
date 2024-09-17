#pragma once

#ifndef _WINDOWS
	#include <cstring>
	#include <cerrno>
	#include <unistd.h>
	#ifdef OSX
	#include <mach/mach_time.h>
	#endif
#else
	#include <winsock2.h>
	#include <ws2tcpip.h>
#endif

#include "if-event.h"

namespace jukey::util
{

//==============================================================================
// 
//==============================================================================
class CommonEvent : public IEvent
{
public:
	CommonEvent();
	~CommonEvent();

	// IEvent
	virtual void Wait(uint64_t) override;
	virtual void Trigger() override;
	virtual void Reset() override;

private:
#ifdef _WINDOWS
	// Windows compability
	typedef HANDLE pthread_t;
	typedef HANDLE pthread_mutex_t;
	typedef HANDLE pthread_cond_t;
	typedef DWORD pthread_key_t;
#endif

	pthread_mutex_t m_event_lock;
	pthread_cond_t m_event_cond;
};

}