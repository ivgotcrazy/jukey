#include "log.h"

using namespace jukey::util;

namespace jukey::sdk
{

SpdlogWrapperSP g_engine_logger = std::make_shared<SpdlogWrapper>("media-engine");

}

