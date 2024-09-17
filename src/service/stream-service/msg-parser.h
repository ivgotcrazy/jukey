#pragma once

#include "stream-common.h"

namespace jukey::srv
{

ServicePingPairOpt ParseServicePing(const com::Buffer& mq_buf,
	const com::Buffer& sig_buf);

PubStreamReqPairOpt ParsePubStreamReq(const com::Buffer& mq_buf,
	const com::Buffer& sig_buf);

UnpubStreamReqPairOpt ParseUnpubStreamReq(const com::Buffer& mq_buf,
	const com::Buffer& sig_buf);

SubStreamReqPairOpt ParseSubStreamReq(const com::Buffer& mq_buf,
	const com::Buffer& sig_buf);

UnsubStreamReqPairOpt ParseUnsubStreamReq(const com::Buffer& mq_buf,
	const com::Buffer& sig_buf);

}