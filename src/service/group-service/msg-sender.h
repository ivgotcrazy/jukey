#pragma once

#include "group-common.h"
#include "config-parser.h"
#include "if-amqp-client.h"
#include "msg-builder.h"

namespace jukey::srv
{

//==============================================================================
// 
//==============================================================================
class MsgSender
{
public:
	MsgSender(const GroupServiceConfig& config, com::IAmqpClient* amqp_client);

	void SendJoinGroupRsp(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const JoinGroupReqPairOpt& req_pair,
		const std::map<uint32_t, UserEntry>& users,
		com::ErrCode result,
		const std::string& msg);

	void SendLeaveGroupRsp(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const LeaveGroupReqPairOpt& req_pair,
		com::ErrCode result,
		const std::string& msg);

	void SendJoinGroupNotify(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const JoinGroupReqPairOpt& req_pair,
		const UserEntry& user_entry);

	void SendLeaveGroupNotify(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const LeaveGroupReqPairOpt& req_pair);

	void SendLeaveGroupNotify(uint32_t app, uint32_t group,
		uint32_t user, uint32_t ut, uint32_t seq);

	void SendPublishMediaRsp(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const PubMediaReqPair& req_pair);

	void SendUnpublishMediaRsp(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const UnpubMediaReqPair& req_pair);

	void SendServicePong(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf);

	void NotifyPublishMedia(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const prot::PublishMediaReq& req,
		uint32_t seq);

	void NotifyUnpublishMedia(const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		const prot::UnpublishMediaReq& req,
		uint32_t seq);

private:
	GroupServiceConfig m_config;
	com::IAmqpClient* m_amqp_client = nullptr;
	MsgBuilderUP m_msg_builder;
};
typedef std::unique_ptr<MsgSender> MsgSenderUP;

}