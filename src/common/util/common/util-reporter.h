#pragma once

#include <string>
#include <memory>
#include <vector>
#include <mutex>

#include "common-struct.h"
#include "if-reporter.h"
#include "com-factory.h"

namespace jukey::util
{

//==============================================================================
// 
//==============================================================================
class HttpReportSender : public com::IReportSender
{
public:
	HttpReportSender(const std::string& host, uint16_t port, 
		const std::string& path);

	virtual void SendReport(const std::string& data) override;

private:
	std::string m_host;
	uint16_t m_port = 0;
	std::string m_path;
};

}