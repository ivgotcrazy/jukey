#include "log.h"

using namespace jukey::util;

namespace jukey::stmr
{

SpdlogWrapperSP g_logger = std::make_shared<SpdlogWrapper>("audio-decode-element");

void LogCallback(void* ptr, int level, const char* fmt, va_list vl)
{
	char buf[1024] = { 0 };
	vsprintf_s(buf, fmt, vl);
	LOG_INF("{}", buf);
}

}

