#pragma once

#include <string>
#include <inttypes.h>

struct ClientParam
{
  std::string addr;
  uint32_t    packet_size = 1024;
  uint32_t    packet_count = 0;
  uint32_t    bitrate = 0;
  uint32_t    packet_rate = 0;
  bool        raw = false;
  bool        reliable = true;
  bool        fec = false;
  uint32_t    log_level = 2;
};

class IClient
{
public:
  virtual bool Init(const ClientParam& param) = 0;

  virtual bool Start() = 0;
};