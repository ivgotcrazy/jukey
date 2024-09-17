#include <iostream>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include "test-spdlog.h"

void TestDefaultLogger()
{
	spdlog::info("============== TestDefaultLogger Begin ==============");

	// Display log level
	spdlog::info("Current log level: {}", spdlog::level::to_string_view(spdlog::get_level()));

	// Log
	spdlog::info("Welcome to spdlog!");
	spdlog::warn("Easy padding in numbers like {:08d}", 12);
	spdlog::error("Some error message with arg: {}", 1);
	spdlog::critical("Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);

	// Change log pattern
	//spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
	/*spdlog::info("Support for floats {:03.2f}", 1.23456);
	spdlog::info("Positional args are {1} {0}..", "too", "supported");
	spdlog::info("{:<30}", "left aligned");*/

	// Set global log level to debug
	spdlog::set_level(spdlog::level::debug); 

	spdlog::info("Current log level: {}", spdlog::level::to_string_view(spdlog::get_level()));

	spdlog::debug("This message should be displayed..");

	// Compile time log levels
	// define SPDLOG_ACTIVE_LEVEL to desired level
	SPDLOG_TRACE("Some trace message with param {}", 42);
	SPDLOG_DEBUG("Some debug message");

	spdlog::info("============== TestDefaultLogger End ==============");
}

void TestStdoutLogger()
{
	spdlog::info("============== TestStdoutLogger Begin ==============");

	auto console = spdlog::stdout_color_mt("console");

	spdlog::get("console")->info("loggers can be retrieved from a global registry using the spdlog::get(logger_name)");

	spdlog::info("============== TestStdoutLogger End ==============");
}

void TestFileLogger()
{
	spdlog::info("============== TestFileLogger Begin ==============");

	// Basic file logger
	try
	{
		auto logger = spdlog::basic_logger_mt("basic_logger", "logs/basic-log.txt");
		logger->info("Basic file log!");
	}
	catch (const spdlog::spdlog_ex & ex)
	{
		std::cout << "Base file logger init failed: " << ex.what() << std::endl;
	}

	// Rotating file logger
	try
	{
		auto max_size = 1048576 * 5;
		auto max_files = 3;
		auto logger = spdlog::rotating_logger_mt("some_logger_name", "logs/rotating.txt", max_size, max_files);
		logger->info("Rotating file log!");
	}
	catch (const spdlog::spdlog_ex & ex)
	{
		std::cout << "Rotating file logger init failed: " << ex.what() << std::endl;
	}

	// Daily file logger
	try
	{
		auto logger = spdlog::daily_logger_mt("daily_logger", "logs/daily.txt", 2, 30);
		logger->info("Daily file log!");
	}
	catch (const spdlog::spdlog_ex& ex)
	{
		std::cout << "Daily file logger init failed: " << ex.what() << std::endl;
	}

	spdlog::info("============== TestFileLogger End ==============");
}

void DoSpdlogTest()
{
	TestDefaultLogger();
	TestStdoutLogger();
	TestFileLogger();
}