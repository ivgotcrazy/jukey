#include "log.h"

using namespace jukey::util;

namespace jukey::dev
{

SpdlogWrapperSP g_logger = std::make_shared<SpdlogWrapper>("device-manager");

}