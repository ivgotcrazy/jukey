#include "test-log.h"

#include "common-struct.h"
#include "log/spdlog-wrapper.h"

using jukey::util::SpdlogWrapper;

SpdlogWrapper* test_log_logger = new SpdlogWrapper("test_log");

//SETUP_MODULE_LOGGER(test_log_logger);
//
//void TestLogWrapper()
//{
//	INF("XXXXXX");
//}