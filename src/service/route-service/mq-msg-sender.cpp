#include "mq-msg-sender.h"
#include "protoc/mq.pb.h"
#include "log.h"
#include "protocol.h"
#include "mq-msg-builder.h"


using namespace jukey::util;
using namespace jukey::com;
using namespace jukey::prot;
using namespace jukey::net;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MqMsgSender::Init(IAmqpClient* amqp_client, uint32_t service_type,
	const std::string& instance_id, const std::string& exchange,
	const std::string& routing_key)
{
	m_amqp_client = amqp_client;
	m_service_type = service_type;
	m_instance_id = instance_id;
	m_route_exchange = exchange;
	m_routing_key = routing_key;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MqMsgSender::SendMqMsg(const std::string& exchange,
	const std::string& routing_key, const Buffer& sig_buf, std::string user_data)
{
	// Build MQ message
	Buffer mq_buf = prot::util::BuildMqMsg(
		MSG_MQ_FROM_ROUTE,
		0,
		m_service_type,
		m_instance_id,
		m_route_exchange,
		"", // routing key
		user_data,
		"", // trace data
		""); // extend data

	// Send message
	m_amqp_client->Publish(exchange, routing_key, sig_buf, mq_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MqMsgSender::SendMqMsg(const std::string& exchange,
	const std::string& routing_key, const Buffer& sig_buf, std::string user_data,
	const std::string& rsp_exchange, const std::string& rsp_routing_key)
{
	// Build MQ message
	Buffer mq_buf = prot::util::BuildMqMsg(
		MSG_MQ_FROM_ROUTE,
		0,
		m_service_type,
		m_instance_id,
		rsp_exchange,
		rsp_routing_key,
		user_data,
		"", // trace data
		""); // extend data

	// Send message
	m_amqp_client->Publish(exchange, routing_key, sig_buf, mq_buf);

	LOG_INF("Send mq message, exchange:{}, routing key:{}, sig len:{}", exchange,
		routing_key, sig_buf.data_len);
}

}