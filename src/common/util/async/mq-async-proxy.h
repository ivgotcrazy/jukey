#pragma once

#include "if-amqp-client.h"
#include "async-proxy-base.h"

namespace jukey::util
{

//==============================================================================
// 
//==============================================================================
class MqAsyncProxy : public AsyncProxyBase
{
public:
	MqAsyncProxy(base::IComFactory* factory,
		com::IAmqpClient* amqp_client, 
		util::IThread* thread, 
		uint32_t timeout);

	//
	// Match response with seq and response message type
	//
	MqDefer& SendMqMsg(
		const std::string& exchange,
		const std::string& routing_key,
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		uint32_t seq,
		uint32_t msg);

	//
	// Match response with seq, message type and user ID
	//
	MqDefer& SendMqMsg(
		const std::string& exchange,
		const std::string& routing_key,
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		uint32_t seq,
		uint32_t msg,
		uint32_t usr);

	//
	// Input message to match response
	//
	bool OnMqMsg(const com::Buffer& mq_buf, const com::Buffer& sig_buf);

private:
	com::IAmqpClient* m_amqp_client = nullptr;
};
typedef std::shared_ptr<MqAsyncProxy> MqAsyncProxySP;

}