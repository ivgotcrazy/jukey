#pragma once

#include <string>

namespace jukey::util
{

class ExecutionTimer
{
public:
	ExecutionTimer(const std::string& name, const std::string& param);
	~ExecutionTimer();

private:
	uint64_t m_start_time = 0;
	std::string m_name;
	std::string m_param;
};

}