#pragma once

#include <inttypes.h>

#include "common-error.h"
#include "common-struct.h"

namespace jukey::sdk
{

class RtcEngineImpl;

//==============================================================================
// 
//==============================================================================
class RegisterProcessor
{
public:
	RegisterProcessor(RtcEngineImpl* engine);

	com::ErrCode Register();
	com::ErrCode Unregister();

private:
	void OnRegisterRsp(const com::Buffer& buf);
	void OnRegisterTimeout();
	void OnRegisterError(com::ErrCode ec);

private:
	RtcEngineImpl* m_rtc_engine = nullptr;
};

}