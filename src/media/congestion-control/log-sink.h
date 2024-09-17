#pragma once

#include "log.h"

#include "rtc_base/logging.h"

namespace jukey::cc
{

//==============================================================================
// 
//==============================================================================
class JukeyLogSink : public rtc::LogSink
{
public:
	void OnLogMessage(const std::string& message, rtc::LoggingSeverity severity,
		const char* tag) override
	{
		switch (severity) {
		case rtc::LoggingSeverity::LS_VERBOSE:
			LOG_DBG("[{}] {}", tag ? tag : "null", message);
			break;
		case rtc::LoggingSeverity::LS_INFO:
			LOG_INF("[{}] {}", tag ? tag : "null", message);
			break;
		case rtc::LoggingSeverity::LS_WARNING:
			LOG_WRN("[{}] {}", tag ? tag : "null", message);
			break;
		case rtc::LoggingSeverity::LS_ERROR:
			LOG_ERR("[{}] {}", tag ? tag : "null", message);
			break;
		default:
			LOG_INF("[{}] {}", tag ? tag : "null", message);
		}
	}

	virtual void OnLogMessage(const std::string& message) override
	{
		OnLogMessage(message, rtc::LoggingSeverity::LS_INFO, nullptr);
	}
};

}


