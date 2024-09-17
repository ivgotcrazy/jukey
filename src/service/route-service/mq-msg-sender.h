#pragma once

#include <string>

#include "common-enum.h"
#include "if-session-mgr.h"
#include "if-amqp-client.h"

namespace jukey::srv
{

//==============================================================================
// 
//==============================================================================
class MqMsgSender
{
public:
	void Init(com::IAmqpClient* amqp_client, 
		uint32_t service_type, 
		const std::string& instance_id,
		const std::string& exchange,
		const std::string& routing_key);

	void SendMqMsg(const std::string& exchange, 
		const std::string& routing_key, 
		const com::Buffer& sig_buf, 
		std::string user_data);

	void SendMqMsg(const std::string& exchange,
		const std::string& routing_key,
		const com::Buffer& sig_buf,
		std::string user_data,
		const std::string& rsp_exchange,
		const std::string& rsp_routing_key);

private:
	com::IAmqpClient* m_amqp_client = nullptr;
	uint32_t m_service_type = 0;
	std::string m_instance_id;
	std::string m_route_exchange;
	std::string m_routing_key;
};

}