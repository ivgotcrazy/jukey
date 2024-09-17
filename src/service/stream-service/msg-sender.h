#pragma once

#include "stream-common.h"
#include "msg-builder.h"
#include "config-parser.h"
#include "if-amqp-client.h"
#include "async/mq-async-proxy.h"

namespace jukey::srv
{

class MsgSender
{
public:
	MsgSender(const StreamServiceConfig& config,
		com::IAmqpClient* amqp_client,
		util::MqAsyncProxySP mq_async_proxy);

	void SendGetParentNodeRsp(
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const prot::GetParentNodeReq& req,
		com::ErrCode result,
		const std::string& msg);

	void SendPubStreamRsp(
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const PubStreamReqPairOpt& req_pair,
		com::ErrCode result,
		const std::string& msg);

	void SendUnpubStreamRsp(
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const UnpubStreamReqPairOpt& req_pair,
		com::ErrCode result,
		const std::string& msg);

	void SendLoginSendChannelNotify(
		const com::MediaStream& stream,
		const std::string& addr,
		uint32_t seq);

	void SendSubStreamRsp(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		SubStreamReqPairOpt req_pair,
		com::ErrCode result,
		const std::string& msg);

	void SendUnsubStreamRsp(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		UnsubStreamReqPairOpt req_pair,
		com::ErrCode result,
		const std::string& msg);

	void SendServicePong(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		ServicePingPairOpt ping_pair);

private:
	com::IAmqpClient* m_amqp_client = nullptr;
	util::MqAsyncProxySP m_mq_async_proxy;
	StreamServiceConfig m_config;
	MsgBuilderUP m_msg_builder;
};
typedef std::unique_ptr<MsgSender> MsgSenderUP;

}