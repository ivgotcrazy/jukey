#include "util-log.h"

namespace jukey::util
{

SpdlogWrapperSP g_util_logger = std::make_shared<SpdlogWrapper>("util");

}