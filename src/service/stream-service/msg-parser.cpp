#include "msg-parser.h"
#include "protocol.h"
#include "log.h"


namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ServicePingPairOpt 
ParseServicePing(const com::Buffer& mq_buf, const com::Buffer& sig_buf)
{
	prot::Ping ping;
	if (!ping.ParseFromArray(PB_PARSE_SIG_PARAM(sig_buf))) {
		LOG_ERR("Parse service ping failed!");
		return std::nullopt;
	}

	prot::MqMsg mq_msg;
	if (!mq_msg.ParseFromArray(PB_PARSE_MQ_PARAM(mq_buf))) {
		LOG_ERR("Parse mq msg failed!");
		return std::nullopt;
	}

	return std::make_optional<ServicePingPair>(mq_msg, ping);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
PubStreamReqPairOpt
ParsePubStreamReq(const com::Buffer& mq_buf, const com::Buffer& sig_buf)
{
	prot::PublishStreamReq req;
	if (!req.ParseFromArray(PB_PARSE_SIG_PARAM(sig_buf))) {
		LOG_ERR("Parse publish stream request failed!");
		return std::nullopt;
	}

	prot::MqMsg mq_msg;
	if (!mq_msg.ParseFromArray(PB_PARSE_MQ_PARAM(mq_buf))) {
		LOG_ERR("Parse mq msg failed!");
		return std::nullopt;
	}

	return std::make_optional<PubStreamReqPair>(mq_msg, req);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
UnpubStreamReqPairOpt
ParseUnpubStreamReq(const com::Buffer& mq_buf, const com::Buffer& sig_buf)
{
	prot::UnpublishStreamReq req;
	if (!req.ParseFromArray(PB_PARSE_SIG_PARAM(sig_buf))) {
		LOG_ERR("Parse unpublish stream request failed!");
		return std::nullopt;
	}

	prot::MqMsg mq_msg;
	if (!mq_msg.ParseFromArray(PB_PARSE_MQ_PARAM(mq_buf))) {
		LOG_ERR("Parse mq msg failed!");
		return std::nullopt;
	}

	return std::make_optional<UnpubStreamReqPair>(mq_msg, req);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SubStreamReqPairOpt
ParseSubStreamReq(const com::Buffer& mq_buf, const com::Buffer& sig_buf)
{
	prot::SubscribeStreamReq req;
	if (!req.ParseFromArray(PB_PARSE_SIG_PARAM(sig_buf))) {
		LOG_ERR("Parse publish stream request failed!");
		return std::nullopt;
	}

	prot::MqMsg mq_msg;
	if (!mq_msg.ParseFromArray(PB_PARSE_MQ_PARAM(mq_buf))) {
		LOG_ERR("Parse from proxy mq msg failed!");
		return std::nullopt;
	}

	return std::make_optional<SubStreamReqPair>(mq_msg, req);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
UnsubStreamReqPairOpt
ParseUnsubStreamReq(const com::Buffer& mq_buf, const com::Buffer& sig_buf)
{
	prot::UnsubscribeStreamReq req;
	if (!req.ParseFromArray(PB_PARSE_SIG_PARAM(sig_buf))) {
		LOG_ERR("Parse unsubscribe stream request failed!");
		return std::nullopt;
	}

	prot::MqMsg mq_msg;
	if (!mq_msg.ParseFromArray(PB_PARSE_MQ_PARAM(mq_buf))) {
		LOG_ERR("Parse from proxy mq msg failed!");
		return std::nullopt;
	}

	return std::make_optional<UnsubStreamReqPair>(mq_msg, req);
}

}