#include "msg-sender.h"
#include "log.h"
#include "protocol.h"

using namespace jukey::com;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MsgSender::MsgSender(const StreamServiceConfig& config, IAmqpClient* amqp_client,
	util::MqAsyncProxySP mq_async_proxy)
	: m_config(config)
	, m_amqp_client(amqp_client)
	, m_mq_async_proxy(mq_async_proxy)
{
	m_msg_builder.reset( new MsgBuilder(m_config.service_config.service_type,
		m_config.service_config.instance_id));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendPubStreamRsp(const Buffer& mq_buf, const Buffer& sig_buf,
	const PubStreamReqPairOpt& req_pair, ErrCode result, const std::string& msg)
{
	MqMsgPairOpt pair = m_msg_builder->BuildPubStreamRspPair(mq_buf, sig_buf,
		req_pair, result, msg);
	if (!pair.has_value()) {
		LOG_ERR("Build publish stream response failed!");
		return; // TODO:
	}

	prot::MqMsg mq_msg;
	if (!mq_msg.ParseFromArray(PB_PARSE_MQ_PARAM(mq_buf))) {
		LOG_ERR("Parse mq msg failed!");
		return;
	}

	// Send publish stream response
	m_amqp_client->Publish(mq_msg.exchange(), mq_msg.routing_key(), pair->second,
		pair->first);

	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(pair->second);

	LOG_INF("Send publish stream response, seq:{}", sig_hdr->seq);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendUnpubStreamRsp(const Buffer& mq_buf, const Buffer& sig_buf, 
	const UnpubStreamReqPairOpt& req_pair, ErrCode result, const std::string& msg)
{
	MqMsgPairOpt pair = m_msg_builder->BuildUnpubStreamRspPair(mq_buf, sig_buf,
		req_pair, result, msg);
	if (!pair.has_value()) {
		LOG_ERR("Build unpublish stream response failed!");
		return; // TODO:
	}

	prot::MqMsg mq_msg;
	if (!mq_msg.ParseFromArray(PB_PARSE_MQ_PARAM(mq_buf))) {
		LOG_ERR("Parse mq msg failed!");
		return;
	}

	// Send publish stream response
	m_amqp_client->Publish(mq_msg.exchange(), mq_msg.routing_key(), pair->second,
		pair->first);

	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(pair->second);

	LOG_INF("Send unpublish stream response, seq:{}", sig_hdr->seq);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendLoginSendChannelNotify(const MediaStream& stream,
	const std::string& addr, uint32_t seq)
{
	MqMsgPair pair = m_msg_builder->BuildLoginSendChannelNotifyPair(seq,
		stream.src.app_id, stream.src.user_id, stream, addr);

	m_mq_async_proxy->SendMqMsg(m_config.route_config.exchange, "", pair.first,
		pair.second, seq, prot::MSG_LOGIN_SEND_CHANNEL_ACK, stream.src.user_id)
		.OnResponse([this](const com::Buffer&, const com::Buffer&) {
			LOG_INF("Received login send channel ack");
		})
		.OnTimeout([this]() {
			LOG_ERR("Send login send channel notify timeout");
		})
		.OnError([](const std::string& err) {
			LOG_ERR("Send login send channel notify error:{}", err);
		});

	LOG_INF("Send login send channel notify to user:{}", stream.src.user_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendGetParentNodeRsp(const Buffer& mq_buf, const Buffer& sig_buf,
	const prot::GetParentNodeReq& req, ErrCode result, const std::string& msg)
{
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	MqMsgPair pair = m_msg_builder->BuildGetParentNodeRspPair(
		mq_buf, sig_buf, req);

	m_amqp_client->Publish("transport-exchange", "", pair.second, pair.first);

	LOG_INF("Send get parent node response to transport service");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendSubStreamRsp(const Buffer& mq_buf, const Buffer& sig_buf,
	SubStreamReqPairOpt req_pair, ErrCode result, const std::string& msg)
{
	MqMsgPair rsp_pair = m_msg_builder->BuildSubStreamRspPair(mq_buf, sig_buf,
		req_pair, result, msg);

	m_amqp_client->Publish(m_config.route_config.exchange, "", rsp_pair.second,
		rsp_pair.first);

	LOG_INF("Send subscribe stream response");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendUnsubStreamRsp(const Buffer& mq_buf, const Buffer& sig_buf,
	UnsubStreamReqPairOpt req_pair, ErrCode result, const std::string& msg)
{
	MqMsgPair rsp_pair = m_msg_builder->BuildUnsubStreamRspPair(mq_buf, sig_buf,
		req_pair, result, msg);

	m_amqp_client->Publish(m_config.route_config.exchange, "", rsp_pair.second,
		rsp_pair.first);

	LOG_INF("Send unsubscribe stream response");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendServicePong(const Buffer& mq_buf, const Buffer& sig_buf,
	ServicePingPairOpt ping_pair)
{
	MqMsgPair pong_pair = m_msg_builder->BuildServicePongPair(mq_buf,
		sig_buf, m_config.service_config);

	m_amqp_client->Publish(ping_pair->first.exchange(),
		ping_pair->first.routing_key(),
		pong_pair.second,
		pong_pair.first);

	LOG_INF("Send service pong, exchange:{}, routing key:{}",
		ping_pair->first.exchange(), ping_pair->first.routing_key());
}

}