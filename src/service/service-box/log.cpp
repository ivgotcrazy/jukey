#include "log.h"

using namespace jukey::util;

namespace jukey::srv
{

SpdlogWrapperSP g_logger = std::make_shared<SpdlogWrapper>("service-box");

}

