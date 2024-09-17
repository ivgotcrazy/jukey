#include "msg-sender.h"
#include "log.h"

using namespace jukey::com;

namespace jukey::srv
{

MsgSender::MsgSender(const UserServiceConfig& config, IAmqpClient* amqp_client)
	: m_config(config), m_amqp_client(amqp_client)
{
	m_msg_builder.reset(new MsgBuilder(m_config.service_config.service_type,
		m_config.service_config.service_name,
		m_config.service_config.instance_id));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendUserLoginRsp(const Buffer& mq_buf, const Buffer& sig_buf,
	UserLoginReqPairOpt req_pair, uint32_t login_id, ErrCode result,
	const std::string& msg)
{
	MqMsgPairOpt rsp_pair = m_msg_builder->BuildUserLoginRspPair(req_pair,
		mq_buf, sig_buf, login_id, result, msg);
	if (!rsp_pair.has_value()) {
		LOG_ERR("Build user login response failed!");
		return; // TODO:
	}

	// Send login response
	m_amqp_client->Publish(m_config.route_config.exchange, "",
		rsp_pair->second, rsp_pair->first);

	LOG_INF("Send login response, exchange:{}", m_config.route_config.exchange);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendUserLogoutRsp(const Buffer& mq_buf, const Buffer& sig_buf,
	UserLogoutReqPairOpt req_pair, ErrCode result, const std::string& msg)
{
	MqMsgPairOpt rsp_pair = m_msg_builder->BuildUserLogoutRspPair(req_pair,
		mq_buf, sig_buf, result, msg);
	if (!rsp_pair.has_value()) {
		LOG_ERR("Build user login response failed!");
		return; // TODO:
	}

	// Send login response
	m_amqp_client->Publish(m_config.route_config.exchange, "",
		rsp_pair->second, rsp_pair->first);

	LOG_INF("Send login response, exchange:{}", m_config.route_config.exchange);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendUserOfflineNotify(uint32_t seq, UserEntrySP entry)
{
	MqMsgPairOpt user_notify = m_msg_builder->BuildUserOfflineNotifyPair(seq,
		entry->app_id, entry->user_id, entry->login_id);

	m_amqp_client->Publish(m_config.service_config.notify_exchange, "",
		user_notify->second, user_notify->first);

	LOG_INF("Send user offline notify, user:{}", entry->user_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendServicePong(const com::Buffer& mq_buf,
	const com::Buffer& sig_buf,
	ServicePingPairOpt ping_pair)
{
	MqMsgPairOpt pong_pair = m_msg_builder->BuildServicePongPair(ping_pair,
		mq_buf, sig_buf);
	if (!pong_pair.has_value()) {
		LOG_ERR("Build service pong failed");
		return;
	}

	m_amqp_client->Publish(ping_pair->first.exchange(),
		ping_pair->first.routing_key(), pong_pair->second, pong_pair->first);

	LOG_INF("Send service pong, exchange:{}, routing key:{}",
		ping_pair->first.exchange(), ping_pair->first.routing_key());
}

}