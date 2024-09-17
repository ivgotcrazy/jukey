#pragma once

#include "common-struct.h"
#include "session-protocol.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class ISendPktNotify
{
public:
	virtual void OnSendPkt(uint32_t data_type, uint32_t data_len) = 0;
};

//==============================================================================
// 
//==============================================================================
class ISessionPktSender
{
public:
	virtual ~ISessionPktSender() {}

	virtual com::ErrCode SendPkt(uint32_t data_type, const com::Buffer& buf) = 0;
};
typedef std::shared_ptr<ISessionPktSender> ISessionPktSenderSP;

}