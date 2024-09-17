#include "execution-timer.h"
#include "common/util-time.h"
#include "log/util-log.h"

namespace jukey::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ExecutionTimer::ExecutionTimer(const std::string& name, const std::string& param)
	: m_name(name), m_param(param)
{
	m_start_time = Now();
}
	
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ExecutionTimer::~ExecutionTimer()
{
	UTIL_INF("{} - {} - execution time:{}", m_name, m_param, Now() - m_start_time);
}

}