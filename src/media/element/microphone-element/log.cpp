#include "log.h"

using namespace jukey::util;

namespace jukey::stmr
{

SpdlogWrapperSP g_logger = std::make_shared<SpdlogWrapper>("microphone-element");

}

