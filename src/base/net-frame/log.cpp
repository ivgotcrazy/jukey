#include "log.h"

using namespace jukey::util;

namespace jukey::net
{

SpdlogWrapperSP g_net_logger = std::make_shared<SpdlogWrapper>("net-frame");

}
