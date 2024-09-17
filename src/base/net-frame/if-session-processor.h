#pragma once

#include "common-struct.h"
#include "session-protocol.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class ISessionProcessor
{
public:
	virtual ~ISessionProcessor() {}

	//
	// Received session packet
	//
	virtual void OnRecvSessionPkt(const SessionPktSP& pkt) = 0;

	//
	// Retrieve reassembled session message
	//
	virtual SessionPktSP GetSessionMsg() = 0;

	//
	// Push session message
	//
	virtual void PushSessionData(const com::Buffer& buf) = 0;

	//
	// Timer
	//
	virtual void OnUpdate() = 0;

	//
	// Next send time
	//
	virtual uint64_t NextSendTime() = 0;

	//
	// Do send session data
	//
	virtual void SendSessionData() = 0;
};
typedef std::shared_ptr<ISessionProcessor> ISessionProcessorSP;

}