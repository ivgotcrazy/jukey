#pragma once

#include "common-struct.h"
#include "common-message.h"

namespace jukey::srv
{

//==============================================================================
// TODO: 
//==============================================================================
enum StreamTransportMsgType
{
	TRANSPORT_MSG_INVALID = SERVICE_TRANSPORT_MSG_START + 0,
	TRANSPORT_MSG_MQ_MSG  = SERVICE_TRANSPORT_MSG_START + 1
};

}