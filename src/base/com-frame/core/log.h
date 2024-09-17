#pragma once

#include "log/spdlog-wrapper.h"

namespace jukey::base
{

extern jukey::util::SpdlogWrapperSP g_base_logger;

#define LOGGER g_base_logger

#define LOG_DBG(...) LOGGER_DBG(__VA_ARGS__)
#define LOG_INF(...) LOGGER_INF(__VA_ARGS__)
#define LOG_WRN(...) LOGGER_WRN(__VA_ARGS__)
#define LOG_ERR(...) LOGGER_ERR(__VA_ARGS__)
#define LOG_CRT(...) LOGGER_CRT(__VA_ARGS__)

}
