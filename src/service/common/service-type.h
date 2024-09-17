#pragma once

#include <optional>

#include "common-struct.h"

namespace jukey::srv
{

typedef std::pair<com::Buffer, com::Buffer> MqMsgPair;
typedef std::optional<MqMsgPair> MqMsgPairOpt;

typedef std::pair<com::Buffer, com::Buffer> SrvMsgPair;
typedef std::optional<SrvMsgPair> SrvMsgPairOpt;

}