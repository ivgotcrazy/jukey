#pragma once

#include "group-common.h"
#include "service-type.h"
#include "protoc/group.pb.h"

namespace jukey::srv
{

//==============================================================================
// 
//==============================================================================
class MsgBuilder
{
public:
	MsgBuilder(uint32_t service_type, const std::string& instance_id);

	MqMsgPair BuildJoinGroupRspPair(
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		JoinGroupReqPairOpt req_pair,
		std::map<uint32_t, UserEntry> users,
		com::ErrCode result,
		const std::string& msg);

	MqMsgPair BuildLeaveGroupRspPair(
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		LeaveGroupReqPairOpt req_pair,
		com::ErrCode result,
		const std::string& msg);

	MqMsgPair BuildJoinGroupNotifyPair(
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		JoinGroupReqPairOpt req_pair,
		const UserEntry& user_entry);

	MqMsgPair BuildLeaveGroupNotifyPair(
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		LeaveGroupReqPairOpt req_pair);

	MqMsgPair BuildLeaveGroupNotifyPair(uint32_t app,
		uint32_t group,
		uint32_t user,
		uint32_t ut,
		uint32_t seq);

	MqMsgPair BuildPublishMediaRspPair(
		const com::Buffer& mq_buf,
		const com::Buffer& prot_buf,
		const PubMediaReqPairOpt& req_pair,
		uint32_t result,
		const std::string& msg);

	MqMsgPair BuildUnpublishMediaRspPair(
		const com::Buffer& mq_buf,
		const com::Buffer& prot_buf,
		const UnpubMediaReqPairOpt& req_pair,
		uint32_t result,
		const std::string& msg);

	MqMsgPair BuildPublishMediaNotifyPair(
		const com::Buffer& mq_buf,
		const com::Buffer& prot_buf,
		const prot::PublishMediaReq& req,
		bool need_ack,
		uint32_t seq);

	MqMsgPair BuildUnpublishMediaNotifyPair(
		const com::Buffer& mq_buf,
		const com::Buffer& prot_buf,
		const prot::UnpublishMediaReq& req,
		bool need_ack,
		uint32_t seq);

	MqMsgPair BuildServicePongPair(
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		uint32_t service_type,
		const std::string& service_name,
		const std::string& instance_id);

private:
	uint32_t m_service_type = 0;
	std::string m_instance_id;
};
typedef std::unique_ptr<MsgBuilder> MsgBuilderUP;

}