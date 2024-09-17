#include "log.h"

using namespace jukey::util;

namespace jukey ::base
{

SpdlogWrapperSP g_base_logger = std::make_shared<SpdlogWrapper>("base-frame");

}

