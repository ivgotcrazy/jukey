#pragma once

#include "log/spdlog-wrapper.h"

namespace jukey::com
{

extern util::SpdlogWrapperSP g_logger;

#define LOG_DBG(...)                                            \
	if (g_logger && g_logger->GetLogger()) {                      \
		SPDLOG_LOGGER_DEBUG(g_logger->GetLogger(), __VA_ARGS__);    \
	}

#define LOG_INF(...)                                            \
	if (g_logger && g_logger->GetLogger()) {                      \
		SPDLOG_LOGGER_INFO(g_logger->GetLogger(), __VA_ARGS__);     \
	}

#define LOG_WRN(...)                                            \
	if (g_logger && g_logger->GetLogger()) {                      \
		SPDLOG_LOGGER_WARN(g_logger->GetLogger(), __VA_ARGS__);     \
	}

#define LOG_ERR(...)                                            \
	if (g_logger && g_logger->GetLogger()) {                      \
		SPDLOG_LOGGER_ERROR(g_logger->GetLogger(), __VA_ARGS__);    \
	}

#define LOG_CRT(...)                                            \
	if (g_logger && g_logger->GetLogger()) {                      \
		SPDLOG_LOGGER_CRITICAL(g_logger->GetLogger(), __VA_ARGS__); \
	}

}
