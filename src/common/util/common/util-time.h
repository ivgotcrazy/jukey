#pragma once

#include <inttypes.h>
#include <string>

namespace jukey::util
{

// Current timestamp, in microsecond
uint64_t Now();

// format: 2023-12-12 12:00:00
std::string NowStr();

// Sleep us microsecond
void Sleep(uint64_t us);

// Sleep to timestamp ts, in microsecond
void SleepTo(uint64_t ts);

}