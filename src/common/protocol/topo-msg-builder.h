#pragma once

#include <string>

#include "common-struct.h"

namespace jukey::prot::util
{

com::Buffer BuildPingMsg(uint32_t seq, 
  const std::string& service_name,
  uint32_t service_type,
  const std::string& instance_id);

com::Buffer BuildPongMsg(uint32_t seq,
  const std::string& service_name,
  uint32_t service_type,
  const std::string& instance_id);

}