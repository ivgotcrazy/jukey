#include "msg-parser.h"
#include "log.h"
#include "protocol.h"

using namespace jukey::com;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ClientOfflineNotifyPairOpt
ParseClientOfflineNotify(const Buffer& mq_buf, const Buffer& sig_buf)
{
	jukey::prot::ClientOfflineNotify notify;
	if (!notify.ParseFromArray(PB_PARSE_SIG_PARAM(sig_buf))) {
		LOG_ERR("Parse client offline notify failed!");
		return std::nullopt;
	}

	prot::MqMsg mq_msg;
	if (!mq_msg.ParseFromArray(PB_PARSE_MQ_PARAM(mq_buf))) {
		LOG_ERR("Parse mq msg failed!");
		return std::nullopt;
	}

	return std::make_optional<ClientOfflineNotifyPair>(mq_msg, notify);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
UserLoginReqPairOpt
ParseUserLoginReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	prot::UserLoginReq login_req;
	if (!login_req.ParseFromArray(PB_PARSE_SIG_PARAM(sig_buf))) {
		LOG_ERR("Parse user login request failed!");
		return std::nullopt;
	}

	prot::MqMsg mq_msg;
	if (!mq_msg.ParseFromArray(PB_PARSE_MQ_PARAM(mq_buf))) {
		LOG_ERR("Parse mq msg failed!");
		return std::nullopt;
	}

	return std::make_optional<UserLoginReqPair>(mq_msg, login_req);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
UserLogoutReqPairOpt
ParseUserLogoutReq(const Buffer& mq_buf, const Buffer& sig_buf)
{
	prot::UserLogoutReq logout_req;
	if (!logout_req.ParseFromArray(PB_PARSE_SIG_PARAM(sig_buf))) {
		LOG_ERR("Parse user logout request failed!");
		return std::nullopt;
	}

	prot::MqMsg mq_msg;
	if (!mq_msg.ParseFromArray(PB_PARSE_MQ_PARAM(mq_buf))) {
		LOG_ERR("Parse mq msg failed!");
		return std::nullopt;
	}

	return std::make_optional<UserLogoutReqPair>(mq_msg, logout_req);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ServicePingPairOpt ParseServicePing(const Buffer& mq_buf, const Buffer& sig_buf)
{
	prot::Ping ping;
	if (!ping.ParseFromArray(PB_PARSE_SIG_PARAM(sig_buf))) {
		LOG_ERR("Parse service ping failed!");
		return std::nullopt;
	}

	prot::MqMsg mq_msg;
	if (!mq_msg.ParseFromArray(PB_PARSE_MQ_PARAM(mq_buf))) {
		LOG_ERR("Parse mq msg failed!");
		return std::nullopt;
	}

	return std::make_optional<ServicePingPair>(mq_msg, ping);
}

}