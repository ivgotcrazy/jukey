#include "log.h"

using namespace jukey::util;

namespace jukey::demo
{

SpdlogWrapperSP g_logger = std::make_shared<SpdlogWrapper>("media-player-demo");

}

