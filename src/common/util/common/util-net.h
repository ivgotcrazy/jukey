#pragma once

#include <string>
#include <optional>

#include "common-struct.h"

namespace jukey::util
{

std::optional<com::Address> ParseAddress(const std::string& str);

}