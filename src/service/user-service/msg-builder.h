#pragma once

#include <optional>

#include "common-struct.h"
#include "user-common.h"
#include "service-type.h"

namespace jukey::srv
{

class MsgBuilder
{
public:
	MsgBuilder(uint32_t service_type,
		const std::string& service_name,
		const std::string& instance);

	MqMsgPairOpt BuildUserLoginRspPair(
		const UserLoginReqPairOpt& req_pair,
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		uint32_t login_id,
		com::ErrCode result,
		const std::string& msg);

	MqMsgPairOpt BuildUserLogoutRspPair(
		const UserLogoutReqPairOpt& req_pair,
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf,
		com::ErrCode result,
		const std::string& msg);

	MqMsgPairOpt BuildUserOfflineNotifyPair(
		uint32_t seq,
		uint32_t app_id,
		uint32_t user_id,
		uint32_t login_id);

	MqMsgPairOpt BuildServicePongPair(
		const ServicePingPairOpt& ping_pair,
		const com::Buffer& mq_buf,
		const com::Buffer& sig_buf);

private:
	uint32_t m_service_type = 0;
	std::string m_service_name;
	std::string m_instance_id;
};
typedef std::unique_ptr<MsgBuilder> MsgBuilderUP;

}