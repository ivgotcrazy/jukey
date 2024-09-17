#pragma once

#include <string>

class IServer
{
public:
  virtual bool Init(const std::string& addr, uint32_t log_level) = 0;
  virtual bool Start() = 0;
};