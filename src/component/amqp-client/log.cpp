#include "log.h"

namespace jukey
{
namespace com
{

util::SpdlogWrapperSP g_logger = std::make_shared<util::SpdlogWrapper>("amqp-client");

}
}

