#pragma once

#include "log/spdlog-wrapper.h"

namespace jukey::net
{

extern jukey::util::SpdlogWrapperSP g_net_logger;

#define LOGGER g_net_logger

#define LOG_DBG(...) LOGGER_DBG(__VA_ARGS__)
#define LOG_INF(...) LOGGER_INF(__VA_ARGS__)
#define LOG_WRN(...) LOGGER_WRN(__VA_ARGS__)
#define LOG_ERR(...) LOGGER_ERR(__VA_ARGS__)
#define LOG_CRT(...) LOGGER_CRT(__VA_ARGS__)

}
