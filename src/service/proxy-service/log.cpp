#include "log.h"

using namespace jukey::util;

namespace jukey::srv
{

SpdlogWrapperSP g_proxy_logger = std::make_shared<SpdlogWrapper>("proxy-service");

}

