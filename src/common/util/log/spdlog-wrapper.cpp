#include <iostream>
#include "spdlog-wrapper.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/daily_file_sink.h"


using namespace spdlog::sinks;

namespace jukey::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SpdlogWrapper::SpdlogWrapper(CSTREF logger_name, spdlog::level::level_enum log_level)
{
	try {
		m_logger_name = logger_name;

		// Make log file full path
		std::string log_path = std::string("./log/").append(logger_name);

		auto file_sink = std::make_shared<daily_file_sink_mt>(log_path, 0, 0);
		file_sink->set_level(spdlog::level::trace);
		file_sink->set_pattern("[%Y-%m-%d %T:%e][%t][%l][%s:%#] %v");

		auto console_sink = std::make_shared<stdout_color_sink_mt>();
		console_sink->set_level(spdlog::level::trace);
		console_sink->set_pattern("[%Y-%m-%d %T:%e][%t][%l][%s:%#] %v");

		std::vector<spdlog::sink_ptr> sinks;
		sinks.push_back(console_sink);
		sinks.push_back(file_sink);

		m_logger = std::make_shared<spdlog::logger>(log_path, begin(sinks),
			end(sinks));

		m_logger->set_level(log_level);

		//m_logger->flush_on(log_level);
	}
	catch (const spdlog::spdlog_ex& ex) {
		std::cout << "Logger init failed: " << ex.what() << std::endl;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::shared_ptr<spdlog::logger> SpdlogWrapper::GetLogger()
{
	return m_logger;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SpdlogWrapper::SetLogLevel(uint8_t level)
{
	if (level <= 6) {
		m_logger->set_level((spdlog::level::level_enum)level);
	}
}

}