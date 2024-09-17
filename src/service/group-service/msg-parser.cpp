#include "msg-parser.h"
#include "log.h"
#include "protocol.h"

using namespace jukey::com;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
JoinGroupReqPairOpt
ParseJoinGroupReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	prot::JoinGroupReq req;
	if (!req.ParseFromArray(PB_PARSE_SIG_PARAM(sig_buf))) {
		LOG_ERR("Parse join group request failed!");
		return std::nullopt;
	}

	prot::MqMsg mq_msg;
	if (!mq_msg.ParseFromArray(PB_PARSE_MQ_PARAM(mq_buf))) {
		LOG_ERR("Parse mq msg failed!");
		return std::nullopt;
	}

	return std::make_optional<JoinGroupReqPair>(mq_msg, req);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
LeaveGroupReqPairOpt
ParseLeaveGroupReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	prot::LeaveGroupReq req;
	if (!req.ParseFromArray(PB_PARSE_SIG_PARAM(sig_buf))) {
		LOG_ERR("Parse leave group request failed!");
		return std::nullopt;
	}

	prot::MqMsg mq_msg;
	if (!mq_msg.ParseFromArray(PB_PARSE_MQ_PARAM(mq_buf))) {
		LOG_ERR("Parse mq msg failed!");
		return std::nullopt;
	}

	return std::make_optional<LeaveGroupReqPair>(mq_msg, req);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
PubMediaReqPairOpt ParsePubMediaReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	prot::PublishMediaReq req;
	if (!req.ParseFromArray(PB_PARSE_SIG_PARAM(sig_buf))) {
		LOG_ERR("Parse publish group stream request failed!");
		return std::nullopt;
	}

	prot::MqMsg mq_msg;
	if (!mq_msg.ParseFromArray(PB_PARSE_MQ_PARAM(mq_buf))) {
		LOG_ERR("Parse publish group stream mq msg failed!");
		return std::nullopt;
	}

	return std::make_optional<PubMediaReqPair>(mq_msg, req);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
UnpubMediaReqPairOpt ParseUnpubMediaReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	prot::UnpublishMediaReq req;
	if (!req.ParseFromArray(PB_PARSE_SIG_PARAM(sig_buf))) {
		LOG_ERR("Parse unpublish media request failed!");
		return std::nullopt;
	}

	prot::MqMsg mq_msg;
	if (!mq_msg.ParseFromArray(PB_PARSE_MQ_PARAM(mq_buf))) {
		LOG_ERR("Parse unpublish media mq msg failed!");
		return std::nullopt;
	}

	return std::make_optional<UnpubMediaReqPair>(mq_msg, req);
}

}