#include "log.h"

using namespace jukey::util;

namespace jukey::txp
{

SpdlogWrapperSP g_txp_logger = std::make_shared<SpdlogWrapper>("transport");

}

