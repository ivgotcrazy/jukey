#pragma once

#include <string>

namespace jukey::srv
{
//==============================================================================
// 
//==============================================================================
class IServiceBox
{
public:
	/**
	 * @brief Initialize service box
	 */
	virtual bool Init(const std::string& config_file) = 0;

	/**
	 * @brief Run service box
	 */
	virtual void Run() = 0;
};

}
