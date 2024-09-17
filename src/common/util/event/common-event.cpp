#include "common-event.h"

#ifndef _WINDOWS
#include <sys/time.h>
#endif


namespace jukey::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
CommonEvent::CommonEvent() : m_event_lock(), m_event_cond()
{
#ifndef _WINDOWS
	pthread_mutex_init(&m_event_lock, NULL);
	pthread_cond_init(&m_event_cond, NULL);
#else
	m_event_lock = CreateMutex(NULL, false, NULL);
	m_event_cond = CreateEvent(NULL, false, false, NULL);
#endif
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
CommonEvent::~CommonEvent()
{
#ifndef _WINDOWS
	pthread_mutex_destroy(&m_event_lock);
	pthread_cond_destroy(&m_event_cond);
#else
	CloseHandle(m_event_lock);
	CloseHandle(m_event_cond);
#endif
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CommonEvent::Wait(uint64_t us)
{
#ifndef _WINDOWS
	timeval now;
	gettimeofday(&now, 0);

	uint64_t second = us / (1000 * 1000);
	uint64_t micsec = us % (1000 * 1000);

	timespec timeout;
	if (now.tv_usec + micsec < 1000 * 1000) {
		timeout.tv_sec = now.tv_sec + second;
		timeout.tv_nsec = (now.tv_usec + micsec) * 1000;
	}
	else {
		timeout.tv_sec = now.tv_sec + second + 1;
		timeout.tv_nsec = (now.tv_usec + micsec - 1000000) * 1000;
	}

	pthread_mutex_lock(&m_event_lock);
	pthread_cond_timedwait(&m_event_cond, &m_event_lock, &timeout);
	pthread_mutex_unlock(&m_event_lock);
#else
	WaitForSingleObject(m_event_cond, (DWORD)(us / 1000));
#endif
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CommonEvent::Trigger()
{
#ifndef _WINDOWS
	pthread_cond_signal(&m_event_cond);
#else
	SetEvent(m_event_cond);
#endif
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void CommonEvent::Reset()
{
#ifdef _WINDOWS
	ResetEvent(m_event_cond);
#endif
}

}