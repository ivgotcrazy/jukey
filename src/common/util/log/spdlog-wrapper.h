#pragma once

#include <memory>
#include <string>

#include "spdlog/spdlog.h"
#include "spdlog/async_logger.h"
#include "spdlog/sinks/base_sink.h"
#include "common-struct.h"


namespace jukey::util
{

//==============================================================================
// 
//==============================================================================
class SpdlogWrapper
{
public:
	SpdlogWrapper(CSTREF logger_name, 
		spdlog::level::level_enum log_level = spdlog::level::info);

	std::shared_ptr<spdlog::logger> GetLogger();

	void SetLogLevel(uint8_t level);

private:
	SpdlogWrapper(const SpdlogWrapper& sw) = delete;
	SpdlogWrapper& operator=(const SpdlogWrapper& sw) = delete;

private:
	std::shared_ptr<spdlog::logger> m_logger;
	std::string m_logger_name;
};
typedef std::shared_ptr<SpdlogWrapper> SpdlogWrapperSP;

}
