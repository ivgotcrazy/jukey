#pragma once

#include <string>

#include "common-struct.h"
#include "protocol.h"

namespace jukey::prot::util
{

com::Buffer BuildMqMsg(
	MsgType msg_type,
	uint32_t seq,
	uint32_t service_type,
	const std::string& instance_id,
	const std::string& exchange,
	const std::string& routing_key,
	const std::string& user_data,
	const std::string& trace_data,
	const std::string& ext_data);

}