#pragma once

#include <optional>

#include "common-struct.h"
#include "common-message.h"
#include "protoc/mq.pb.h"
#include "protoc/terminal.pb.h"

namespace jukey::srv
{

typedef std::tuple<prot::MqMsg, prot::RegisterReq, prot::RegisterReqExtendData> RegisterReqTuple;
typedef std::optional<RegisterReqTuple> RegisterReqTupleOpt;

typedef std::pair<prot::MqMsg, prot::UnregisterReq> UnregisterReqPair;
typedef std::optional<UnregisterReqPair> UnregisterReqPairOpt;

//==============================================================================
// TODO: 
//==============================================================================
enum TerminalMsgType
{
	TERMINAL_MSG_INVALID = SERVICE_TERMINAL_MSG_START + 0,
	TERMINAL_MSG_MQ_MSG  = SERVICE_TERMINAL_MSG_START + 1
};

}