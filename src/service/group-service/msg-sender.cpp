#include "msg-sender.h"
#include "log.h"
#include "service-type.h"
#include "protocol.h"

using namespace jukey::com;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MsgSender::MsgSender(const GroupServiceConfig& config, IAmqpClient* amqp_client)
	: m_config(config), m_amqp_client(amqp_client)
{
	m_msg_builder.reset(new MsgBuilder(m_config.service_config.service_type,
		m_config.service_config.instance_id));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendJoinGroupRsp(const Buffer& mq_buf, const Buffer& sig_buf,
	const JoinGroupReqPairOpt& req_pair, 
	const std::map<uint32_t, UserEntry>& users, 
	ErrCode result,
	const std::string& msg)
{
	MqMsgPair rsp_pair = m_msg_builder->BuildJoinGroupRspPair(
		mq_buf, sig_buf, req_pair, users, result, msg);

	m_amqp_client->Publish(m_config.route_config.exchange, "", rsp_pair.second,
		rsp_pair.first);

	LOG_INF("Send join group response, result:{}", result);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendLeaveGroupRsp(const Buffer& mq_buf, const Buffer& sig_buf,
	const LeaveGroupReqPairOpt& req_pair, ErrCode result, const std::string& msg)
{
	MqMsgPair rsp_pair = m_msg_builder->BuildLeaveGroupRspPair(
		mq_buf, sig_buf, req_pair, result, msg);

	m_amqp_client->Publish(m_config.route_config.exchange, "", rsp_pair.second,
		rsp_pair.first);

	LOG_INF("Send join group response, result:{}", result);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendPublishMediaRsp(const Buffer& mq_buf, const Buffer& sig_buf,
	const PubMediaReqPair& req_pair)
{
	MqMsgPair send_rsp_pair = m_msg_builder->BuildPublishMediaRspPair(
		mq_buf, sig_buf, req_pair, ERR_CODE_OK, "success");

	// Send response
	m_amqp_client->Publish(m_config.route_config.exchange, "",
		send_rsp_pair.second, send_rsp_pair.first);

	LOG_INF("Send publish media response");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendUnpublishMediaRsp(const Buffer& mq_buf,
	const Buffer& sig_buf, const UnpubMediaReqPair& req_pair)
{
	MqMsgPair send_rsp_pair = m_msg_builder->BuildUnpublishMediaRspPair(
		mq_buf, sig_buf, req_pair, ERR_CODE_OK, "success");

	// Send response
	m_amqp_client->Publish(m_config.route_config.exchange, "",
		send_rsp_pair.second, send_rsp_pair.first);

	LOG_INF("Send unpublish media response");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendJoinGroupNotify(const Buffer& mq_buf, const Buffer& sig_buf,
	const JoinGroupReqPairOpt& req_pair, const UserEntry& user_entry)
{
	auto notify = m_msg_builder->BuildJoinGroupNotifyPair(mq_buf, sig_buf,
		req_pair, user_entry);

	m_amqp_client->Publish(m_config.route_config.exchange, "", notify.second,
		notify.first);

	LOG_INF("Send join group notify");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendLeaveGroupNotify(const Buffer& mq_buf, const Buffer& sig_buf,
	const LeaveGroupReqPairOpt& req_pair)
{
	auto notify = m_msg_builder->BuildLeaveGroupNotifyPair(mq_buf, sig_buf,
		req_pair);

	m_amqp_client->Publish(m_config.route_config.exchange, "", notify.second,
		notify.first);

	LOG_INF("Send user leave group notify");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendLeaveGroupNotify(uint32_t app, uint32_t group,
	uint32_t user, uint32_t ut, uint32_t seq)
{
	auto notify = m_msg_builder->BuildLeaveGroupNotifyPair(app, group, user, ut, 
		seq);

	m_amqp_client->Publish(m_config.route_config.exchange, "", notify.second,
		notify.first);

	LOG_INF("Send user leave group notify");
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

	auto pair = m_msg_builder->BuildServicePongPair(mq_buf, sig_buf,
		m_config.service_config.service_type,
		m_config.service_config.service_name,
		m_config.service_config.instance_id);

	m_amqp_client->Publish(mq_msg.exchange(), mq_msg.routing_key(), pair.second,
		pair.first);

	LOG_DBG("Send service pong message, exchange:{}, routing key:{}",
		mq_msg.exchange(), mq_msg.routing_key());
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::NotifyUnpublishMedia(const com::Buffer& mq_buf,
	const com::Buffer& sig_buf, const prot::UnpublishMediaReq& req, uint32_t seq)
{
	MqMsgPair pair = m_msg_builder->BuildUnpublishMediaNotifyPair(
		mq_buf, sig_buf, req, true, seq);

	m_amqp_client->Publish("route-exchange", "", pair.second, pair.first);

	LOG_INF("Send unpublish media notify");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::NotifyPublishMedia(const com::Buffer& mq_buf,
	const com::Buffer& sig_buf, const prot::PublishMediaReq& req, uint32_t seq)
{
	MqMsgPair pair = m_msg_builder->BuildPublishMediaNotifyPair(
		mq_buf, sig_buf, req, true, seq);

	m_amqp_client->Publish("route-exchange", "", pair.second, pair.first);

	LOG_INF("Send publish media notify");
}

}