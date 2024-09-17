#pragma once

#include "terminal-common.h"

namespace jukey::srv
{

RegisterReqTupleOpt ParseRegisterReq(const com::Buffer& mq_buf,
	const com::Buffer& sig_buf);

UnregisterReqPairOpt ParseUnregisterReq(const com::Buffer& mq_buf,
	const com::Buffer& sig_buf);

}