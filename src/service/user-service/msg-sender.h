#pragma once

#include "user-common.h"
#include "msg-builder.h"
#include "config-parser.h"
#include "if-amqp-client.h"


namespace jukey::srv
{

//==============================================================================
//
//==============================================================================
class MsgSender
{
public:
	MsgSender(const UserServiceConfig& config, com::IAmqpClient* amqp_client);

	void SendUserOfflineNotify(uint32_t seq, UserEntrySP entry);

	void SendUserLoginRsp(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		UserLoginReqPairOpt req_pair,
		uint32_t login_id,
		com::ErrCode result,
		const std::string& msg);

	void SendUserLogoutRsp(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		UserLogoutReqPairOpt req_pair,
		com::ErrCode result,
		const std::string& msg);

	void SendServicePong(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		ServicePingPairOpt ping_pair);

private:
	com::IAmqpClient* m_amqp_client = nullptr;
	MsgBuilderUP m_msg_builder;
	UserServiceConfig m_config;
};
typedef std::unique_ptr<MsgSender> MsgSenderUP;

}