#pragma once

#include "common-struct.h"
#include "session-protocol.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class ISessionPktAssembler
{
public:
	virtual ~ISessionPktAssembler() {}

	virtual void InputSessionData(const com::Buffer& buf) = 0;

	virtual SessionPktSP GetNextSessionPkt() = 0;
};
typedef std::shared_ptr<ISessionPktAssembler> ISessionPktAssemblerSP;

}