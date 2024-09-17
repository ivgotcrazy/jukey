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
class LoginProcessor
{
public:
	LoginProcessor(RtcEngineImpl* engine);

	com::ErrCode Login(uint32_t user_id);
	com::ErrCode Logout();

private:
	void OnUserLoginRsp(const com::Buffer& buf);
	void OnUserLoginTimeout();
	void OnUserLoginError(com::ErrCode ec);

	void OnUserLogoutRsp(const com::Buffer& buf);
	void OnUserLogoutTimeout();
	void OnUserLogoutError(com::ErrCode ec);

private:
	RtcEngineImpl* m_rtc_engine = nullptr;
};

}