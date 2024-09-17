#pragma once

#include "if-amqp-client.h"
#include "config-parser.h"
#include "msg-builder.h"

namespace jukey::srv
{

class MsgSender
{
public:
	MsgSender(com::IAmqpClient* amqp_client,
		const TerminalServiceConfig& config);

	void SendRegisterRsp(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const prot::RegisterReq& req,
		uint32_t register_id,
		com::ErrCode result,
		const std::string& msg);

	void SendUnregisterRsp(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const prot::UnregisterReq& req,
		com::ErrCode result,
		const std::string& msg);

	void SendServicePong(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf);

	void SendClientOfflineRsp(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const prot::ClientOfflineReq& req);

	void SendClientOfflineNotify(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const prot::ClientOfflineReq& req,
		uint32_t register_id);

private:
	com::IAmqpClient* m_amqp_client = nullptr;
	TerminalServiceConfig m_config;
	MsgBuilderUP m_msg_builder;
};
typedef std::unique_ptr<MsgSender> MsgSenderUP;

}