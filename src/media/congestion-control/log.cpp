#include "log.h"

using namespace jukey::util;

namespace jukey::cc
{

SpdlogWrapperSP g_cc_logger = std::make_shared<SpdlogWrapper>("congestion-control");

}

