#include "log.h"

using namespace jukey::util;

namespace jukey::media::util
{

SpdlogWrapperSP g_logger = std::make_shared<SpdlogWrapper>("media-util");

}