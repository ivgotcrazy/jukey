#pragma once

#include "common-message.h"

namespace jukey::srv
{

//==============================================================================
// 
//==============================================================================
enum RouteMsgType
{
	ROUTE_MSG_MQ_MSG = SERVICE_ROUTE_MSG_START + 1,
	ROUTE_MSG_SEND_PING,
	ROUTE_MSG_PING_RESULT,
};

//==============================================================================
// 
//==============================================================================
struct SendPingData
{
	SendPingData(const com::ServiceParam& p, const com::Buffer b)
		: param(p), buf(b) {}

	com::ServiceParam param;
	com::Buffer buf;
};
typedef std::shared_ptr<SendPingData> SendPingDataSP;

//==============================================================================
// 
//==============================================================================
struct PingResultData
{
	PingResultData(const com::ServiceParam& p, bool r) : param(p), result(r) {}

	com::ServiceParam param;
	bool result = false;
};
typedef std::shared_ptr<PingResultData> PingResultDataSP;

}