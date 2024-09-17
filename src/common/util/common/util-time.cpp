#include "util-time.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

#ifndef _WINDOWS
  #include <cstring>
  #include <cerrno>
  #include <unistd.h>
  #include <time.h>
  #include <sys/time.h>
  #ifdef OSX
	#include <mach/mach_time.h>
  #endif
#else
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #ifdef LEGACY_WIN32
	#include <wspiapi.h>
  #endif
#endif

#ifdef _WINDOWS
#pragma comment(lib, "winmm.lib")
#endif

// Forward declaration
namespace jukey::util
{
  uint64_t ReadCPUFrequency();
}

// Intialize, tick / us
static uint64_t s_cpu_frequency = jukey::util::ReadCPUFrequency();

// Use high precision(cpu tick) default
static bool s_use_low_precision = false;

namespace jukey::util
{

//------------------------------------------------------------------------------
// UDT impelementation
//------------------------------------------------------------------------------
uint64_t GetTime()
{
  // For Cygwin and other systems without microsecond level resolution, 
  // uncomment the following three lines
  //uint64_t x;
  //rdtsc(x);
  //return x / s_ullCPUFrequency;
  //Specific fix may be necessary if rdtsc is not available either.

#ifndef _WINDOWS
  timeval t;
  gettimeofday(&t, 0);
  return t.tv_sec * 1000000ULL + t.tv_usec;
#else
  LARGE_INTEGER ccf;
  HANDLE hCurThread = ::GetCurrentThread();
  DWORD_PTR dwOldMask = ::SetThreadAffinityMask(hCurThread, 1);
  if (QueryPerformanceFrequency(&ccf))
  {
	  LARGE_INTEGER cc;
	  if (QueryPerformanceCounter(&cc))
	  {
	    SetThreadAffinityMask(hCurThread, dwOldMask);
	    return (cc.QuadPart * 1000000ULL / ccf.QuadPart);
	  }
  }

  SetThreadAffinityMask(hCurThread, dwOldMask);
  return GetTickCount64() * 1000ULL;
#endif
}

//------------------------------------------------------------------------------
// UDT impelementation
//------------------------------------------------------------------------------
uint64_t ReadCPUFrequency()
{
  uint64_t frequency = 1;  // 1 tick per microsecond.

#if defined(IA32) || defined(IA64) || defined(AMD64)
  uint64_t t1, t2;

  rdtsc(t1);
  timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 100000000;
  nanosleep(&ts, NULL);
  rdtsc(t2);

  // CPU clocks per microsecond
  frequency = (t2 - t1) / 100000;
#elif defined(_WINDOWS)
  int64_t ccf;
  if (QueryPerformanceFrequency((LARGE_INTEGER*)&ccf))
	  frequency = ccf / 1000000;
#elif defined(OSX)
  mach_timebase_info_data_t info;
  mach_timebase_info(&info);
  frequency = info.denom * 1000ULL / info.numer;
#endif

  // Fall back to microsecond if the resolution is not high enough.
  if (frequency < 10) {
	  frequency = 1;
	  s_use_low_precision = true;
  }

  return frequency;
}

//------------------------------------------------------------------------------
// UDT impelementation
//------------------------------------------------------------------------------
uint64_t Now()
{
  uint64_t ts = 0;

  if (s_use_low_precision) {
	  ts = GetTime();
  }
  else {
#ifdef IA32
	  uint32_t lval, hval;
	  asm volatile ("rdtsc" : "=a" (lval), "=d" (hval));
	  ts = hval;
	  ts = (ts << 32) | lval;
#elif defined(IA64)
	  asm("mov %0=ar.itc" : "=r"(ts) :: "memory");
#elif defined(AMD64)
	  uint32_t lval, hval;
	  asm("rdtsc" : "=a" (lval), "=d" (hval));
	  ts = hval;
	  ts = (ts << 32) | lval;
#elif defined(_WINDOWS)
	  BOOL ret = QueryPerformanceCounter((LARGE_INTEGER*)&ts);
	  if (!ret)
	    ts = GetTime() * s_cpu_frequency;
#elif defined(OSX)
	  ts = mach_absolute_time();
#else
	  // use system call to read time clock for other archs
	  ts = GetTime();
#endif
  }

  return ts / s_cpu_frequency;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SleepTo(uint64_t ts)
{
#ifndef NO_BUSY_WAITING
  while (Now() < ts) {
#ifdef IA32
	__asm__ volatile ("pause; rep; nop; nop; nop; nop; nop;");
#elif IA64
	__asm__ volatile ("nop 0; nop 0; nop 0; nop 0; nop 0;");
#elif AMD64
	__asm__ volatile ("nop; nop; nop; nop; nop;");
#endif
  }
#else
#ifndef _WINDOWS
  pthread_mutex_init(&tick_lock, NULL);
  pthread_cond_init(&tick_cond, NULL);
  while (Now() < ts) {
	timeval now;
	timespec timeout;
	gettimeofday(&now, 0);
	if (now.tv_usec < 990000)
	{
	  timeout.tv_sec = now.tv_sec;
	  timeout.tv_nsec = (now.tv_usec + 10000) * 1000;
	}
	else
	{
	  timeout.tv_sec = now.tv_sec + 1;
	  timeout.tv_nsec = (now.tv_usec + 10000 - 1000000) * 1000;
	}
	pthread_mutex_lock(&tick_lock);
	pthread_cond_timedwait(&tick_cond, &tick_lock, &timeout);
	pthread_mutex_unlock(&tick_cond);
  }
  pthread_mutex_destroy(&tick_lock);
  pthread_cond_destroy(&tick_cond);
#else
	// 设置定时器分辨率为 1 毫秒
	timeBeginPeriod(1);

  HANDLE tick_event = CreateEvent(NULL, false, false, NULL);
  if (tick_event != 0) {
	  while (Now() < ts) {
	    WaitForSingleObject(tick_event, 1);
	  }
	  CloseHandle(tick_event);
  }

	// 恢复系统定时器分辨率
	timeEndPeriod(1);
#endif
#endif
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void Sleep(uint64_t us)
{
  SleepTo(Now() + us);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string NowStr()
{
  // 获取当前时间
  auto now = std::chrono::system_clock::now();
  std::time_t now_c = std::chrono::system_clock::to_time_t(now);

  // 将时间格式化为字符串
  std::ostringstream oss;
  struct tm time_info;
  localtime_s(&time_info, &now_c);
  oss << std::put_time(&time_info, "%Y-%m-%d %H:%M:%S");

  return oss.str();
}

}
