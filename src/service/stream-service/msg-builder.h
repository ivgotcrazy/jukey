#pragma once

#include "service-type.h"
#include "config-parser.h"
#include "stream-common.h"

namespace jukey::srv
{

//==============================================================================
// 
//==============================================================================
class MsgBuilder
{
public:
	MsgBuilder(uint32_t service_type, const std::string& instance_id);

	MqMsgPair BuildServicePongPair(
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const ServiceConfig& config);

	MqMsgPair BuildPubStreamRspPair(
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const PubStreamReqPairOpt& req_pair,
		com::ErrCode result,
		const std::string& msg);

	MqMsgPair BuildUnpubStreamRspPair(
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const UnpubStreamReqPairOpt& req_pair,
		com::ErrCode result,
		const std::string& msg);

	MqMsgPair BuildSubStreamRspPair(
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const SubStreamReqPairOpt& req_pair,
		com::ErrCode result,
		const std::string& msg);

	MqMsgPair BuildUnsubStreamRspPair(
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const UnsubStreamReqPairOpt& req_pair,
		com::ErrCode result,
		const std::string& msg);

	MqMsgPair BuildLoginSendChannelNotifyPair(
		uint32_t seq,
		uint32_t app_id,
		uint32_t user_id,
		const com::MediaStream& stream,
		const std::string& addr);

	MqMsgPair BuildLoginSendChannelAckPair(
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const prot::LoginSendChannelNotify& notify,
		uint32_t result,
		const std::string& msg);

	MqMsgPair BuildGetParentNodeRspPair(
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const prot::GetParentNodeReq& req);

private:
	uint32_t m_service_type = 0;
	std::string m_instance_id;
};
typedef std::unique_ptr<MsgBuilder> MsgBuilderUP;

}