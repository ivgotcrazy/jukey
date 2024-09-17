#pragma once

#include "group-common.h"

namespace jukey::srv
{

PubMediaReqPairOpt ParsePubMediaReq(
	const com::Buffer& mq_buf,
	const com::Buffer& sig_buf);

UnpubMediaReqPairOpt ParseUnpubMediaReq(
	const com::Buffer& mq_buf,
	const com::Buffer& sig_buf);

JoinGroupReqPairOpt ParseJoinGroupReq(
	const com::Buffer& mq_buf,
	const com::Buffer& sig_buf);

LeaveGroupReqPairOpt ParseLeaveGroupReq(
	const com::Buffer& mq_buf,
	const com::Buffer& sig_buf);

}