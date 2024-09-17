#include "msg-sender.h"
#include "log.h"
#include "protoc/mq.pb.h"
#include "protocol.h"

using namespace jukey::com;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MsgSender::MsgSender(IAmqpClient* amqp_client,
	const TerminalServiceConfig& config)
	: m_amqp_client(amqp_client), m_config(config)
{
	m_msg_builder.reset(new MsgBuilder(m_config));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendRegisterRsp(const Buffer& mq_buf, const Buffer& sig_buf,
	const prot::RegisterReq& req, uint32_t register_id, ErrCode result,
	const std::string& msg)
{
	MqMsgPair rsp_pair = m_msg_builder->BuildRegRsp(mq_buf, sig_buf, req,
		register_id, result, msg);

	m_amqp_client->Publish(m_config.route_config.exchange, "",
		rsp_pair.second, rsp_pair.first);

	LOG_INF("Send register response");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendUnregisterRsp(const Buffer& mq_buf, const Buffer& sig_buf,
	const prot::UnregisterReq& req, ErrCode result, const std::string& msg)
{
	MqMsgPair rsp_pair = m_msg_builder->BuildUnregRsp(mq_buf, sig_buf, req,
		result, msg);

	m_amqp_client->Publish(m_config.route_config.exchange, "",
		rsp_pair.second, rsp_pair.first);

	LOG_INF("Send unregister response");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendServicePong(const Buffer& mq_buf, const Buffer& sig_buf)
{
	prot::MqMsg mq_msg;
	if (!mq_msg.ParseFromArray(PB_PARSE_MQ_PARAM(mq_buf))) {
		LOG_ERR("Parse mq msg failed!");
		return;
	}

	MqMsgPair pong_pair = m_msg_builder->BuildServicePong(mq_buf, sig_buf);

	m_amqp_client->Publish(mq_msg.exchange(), mq_msg.routing_key(),
		pong_pair.second, pong_pair.first);

	LOG_INF("Send service pong, exchange:{}, routing key:{}", mq_msg.exchange(),
		mq_msg.routing_key());
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendClientOfflineRsp(const Buffer& mq_buf, const Buffer& sig_buf,
	const prot::ClientOfflineReq& req)
{
	MqMsgPair rsp_pair = m_msg_builder->BuildClientOfflineRspPair(mq_buf, sig_buf,
		req);

	m_amqp_client->Publish(m_config.route_config.exchange, "", rsp_pair.second,
		rsp_pair.first);

	LOG_INF("Send client offline response");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendClientOfflineNotify(const Buffer& mq_buf,
	const Buffer& sig_buf, const prot::ClientOfflineReq& req, uint32_t register_id)
{
	MqMsgPair notify_pair = m_msg_builder->BuildClientOfflineNotifyPair(mq_buf, 
		sig_buf, req, register_id);

	m_amqp_client->Publish(m_config.service_config.notify_exchange, "",
		notify_pair.second, notify_pair.first);

	LOG_INF("Send client offline notify");
}

}