#pragma once

#include <string>

#include "if-unknown.h"
#include "common-enum.h"
#include "common-struct.h"

namespace jukey::com
{

#define CID_AMQP_CLIENT "cid-amqp-client"
#define IID_AMQP_CLIENT "iid-amqp-client"

//==============================================================================
// 
//==============================================================================
class IAmqpHandler
{
public:
	virtual void OnRecvMqMsg(const std::string& queue,
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf) = 0;
};

//==============================================================================
// 
//==============================================================================
struct AmqpParam
{
	std::string owner;
	std::string host;
	std::string user;
	std::string pwd;
	uint16_t port = 0;
	IAmqpHandler* handler = nullptr;
};

//==============================================================================
// 
//==============================================================================
enum class ExchangeType
{
	FANOUT = 0,
	DIRECT = 1,
	TOPIC  = 2
};

//==============================================================================
// 
//==============================================================================
class IAmqpClient : public base::IUnknown
{
public:
	//
	// Initialize
	//
	virtual com::ErrCode Init(const AmqpParam& param) = 0;

	//
	// Create exchange
	//
	virtual com::ErrCode DeclareExchange(const std::string& name, 
		ExchangeType type) = 0;

	//
	// Remove exchange
	//
	virtual com::ErrCode RemoveExchange(const std::string& name) = 0;

	//
	// Create queue
	//
	virtual com::ErrCode DeclareQueue(const std::string& name) = 0;

	//
	// Remove queue
	//
	virtual com::ErrCode RemoveQueue(const std::string& name) = 0;

	//
	// Bind queue to exchange (need remove?)
	//
	virtual com::ErrCode BindQueue(const std::string& exchange, 
		const std::string& queue, 
		const std::string& bind_key) = 0;

	//
	// Send message to exchange with routing_key
	//
	virtual com::ErrCode Publish(const std::string& exchange,
		const std::string& routing_key,
		const com::Buffer& sig_buf,
		const com::Buffer& mq_buf) = 0;

	//
	// Send message to exchange with routing_key
	//
	virtual com::ErrCode Publish(const std::string& exchange,
	const std::string& routing_key,
	const com::Buffer& buf) = 0;
};

}