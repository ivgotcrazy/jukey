#pragma once

#include <string>

#include "common-enum.h"

namespace jukey::sdk
{

std::string MakeStreamSrcId(const std::string& addr,
	const std::string& user_id,
	uint32_t stream_type,
	const std::string& stream_id);

}