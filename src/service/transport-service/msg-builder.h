#pragma once

#include <optional>

#include "common-struct.h"
#include "service-type.h"
#include "protoc/transport.pb.h"

namespace jukey::srv
{

MqMsgPair BuildGetParentNodeReqPair(
	uint32_t seq,
	const prot::LoginRecvChannelReq& req, 
	const std::string& addr,
	const std::string& exchange,
	uint32_t service_type,
	const std::string& instance_id);

}