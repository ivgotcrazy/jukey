#pragma once

#include "spdlog-wrapper.h"

namespace jukey::util
{

extern jukey::util::SpdlogWrapperSP g_util_logger;

#define UTIL_DBG(...)                                                \
  if (g_util_logger && g_util_logger->GetLogger()) {                 \
    SPDLOG_LOGGER_DEBUG(g_util_logger->GetLogger(), __VA_ARGS__);    \
  }

#define UTIL_INF(...)                                                \
  if (g_util_logger && g_util_logger->GetLogger()) {                 \
    SPDLOG_LOGGER_INFO(g_util_logger->GetLogger(), __VA_ARGS__);     \
  }

#define UTIL_WRN(...)                                                \
  if (g_util_logger && g_util_logger->GetLogger()) {                 \
    SPDLOG_LOGGER_WARN(g_util_logger->GetLogger(), __VA_ARGS__);     \
  }

#define UTIL_ERR(...)                                                \
  if (g_util_logger && g_util_logger->GetLogger()) {                 \
    SPDLOG_LOGGER_ERROR(g_util_logger->GetLogger(), __VA_ARGS__);    \
  }

#define UTIL_CRT(...)                                                \
  if (g_util_logger && g_util_logger->GetLogger()) {                 \
    SPDLOG_LOGGER_CRITICAL(g_util_logger->GetLogger(), __VA_ARGS__); \
  }

}