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
class JoinProcessor
{
public:
	JoinProcessor(RtcEngineImpl* engine);

	com::ErrCode JoinGroup(uint32_t group_id);
	com::ErrCode LeaveGroup();

private:
	void OnJoinGroupRsp(const com::Buffer& buf);
	void OnJoinGroupTimeout();
	void OnJoinGroupError(com::ErrCode ec);

	void OnLeaveGroupRsp(const com::Buffer& buf);
	void OnLeaveGroupTimeout();
	void OnLeaveGroupError(com::ErrCode ec);

private:
	RtcEngineImpl* m_rtc_engine = nullptr;
};

}