#include "log.h"

using namespace jukey::util;

namespace jukey::com
{

SpdlogWrapperSP g_logger = std::make_shared<SpdlogWrapper>("reporter");

}

