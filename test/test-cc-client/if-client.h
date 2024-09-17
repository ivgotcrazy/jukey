#pragma once

#include <string>
#include <inttypes.h>

struct ClientParam
{
  std::string addr;
  uint32_t    log_level = 2;
};

class IClient
{
public:
  virtual bool Init(const ClientParam& param) = 0;

  virtual bool Start() = 0;
};