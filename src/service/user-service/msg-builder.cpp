#include "msg-builder.h"
#include "protocol.h"
#include "user-msg-builder.h"
#include "topo-msg-builder.h"
#include "mq-msg-builder.h"

using namespace jukey::com;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MsgBuilder::MsgBuilder(uint32_t service_type, const std::string& service_name,
	const std::string& instance)
	: m_service_type(service_type)
	, m_service_name(service_name)
	, m_instance_id(instance)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPairOpt MsgBuilder::BuildUserLoginRspPair(
	const UserLoginReqPairOpt& req_pair,
	const Buffer& mq_buf, 
	const Buffer& sig_buf,
	uint32_t login_id,
	ErrCode result, 
	const std::string& msg)
{
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	prot::util::UserLoginRspParam rsp_param;
	rsp_param.app_id = req_pair->second.app_id();
	rsp_param.client_id = req_pair->second.client_id();
	rsp_param.register_id = req_pair->second.register_id();
	rsp_param.user_id = req_pair->second.user_id();
	rsp_param.user_type = req_pair->second.user_type();
	rsp_param.login_id = login_id;
	rsp_param.result = result;
	rsp_param.msg = msg;

	com::SigHdrParam hdr_param;
	hdr_param.app_id = req_pair->second.app_id();
	hdr_param.user_id = req_pair->second.user_id();
	hdr_param.client_id = sig_hdr->clt;
	hdr_param.seq = sig_hdr->seq;

	// Build register response
	Buffer send_sig_buf = BuildUserLoginRsp(rsp_param, hdr_param);

	// Check MQ message type
	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)DP(mq_buf);

	// Build MQ message
	Buffer send_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_TO_ROUTE,
		mq_hdr->seq,
		m_service_type,
		m_instance_id,
		"",
		"",
		req_pair.value().first.user_data(),
		"", // trace data
		""); // extend data

	return std::make_optional<MqMsgPair>(send_mq_buf, send_sig_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPairOpt MsgBuilder::BuildUserLogoutRspPair(
	const UserLogoutReqPairOpt& req_pair,
	const Buffer& mq_buf,
	const Buffer& sig_buf,
	ErrCode result,
	const std::string& msg)
{
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	prot::util::UserLogoutRspParam rsp_param;
	rsp_param.app_id = req_pair->second.app_id();
	rsp_param.client_id = req_pair->second.client_id();
	rsp_param.register_id = req_pair->second.register_id();
	rsp_param.user_id = req_pair->second.user_id();
	rsp_param.user_type = req_pair->second.user_type();
	rsp_param.login_id = req_pair->second.login_id();
	rsp_param.result = result;
	rsp_param.msg = msg;

	com::SigHdrParam hdr_param;
	hdr_param.app_id = req_pair->second.app_id();
	hdr_param.user_id = req_pair->second.user_id();
	hdr_param.client_id = sig_hdr->clt;
	hdr_param.seq = sig_hdr->seq;

	// Build register response
	Buffer send_sig_buf = BuildUserLogoutRsp(rsp_param, hdr_param);

	// Check MQ message type
	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)DP(mq_buf);

	// Build MQ message
	Buffer send_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_TO_ROUTE,
		mq_hdr->seq,
		m_service_type,
		m_instance_id,
		"",
		"",
		req_pair.value().first.user_data(),
		"", // trace data
		""); // extend data

	return std::make_optional<MqMsgPair>(send_mq_buf, send_sig_buf);
}

//------------------------------------------------------------------------------
// MQ seq as the same with signal seq
//------------------------------------------------------------------------------
MqMsgPairOpt MsgBuilder::BuildUserOfflineNotifyPair(uint32_t seq,
	uint32_t app_id, uint32_t user_id, uint32_t login_id)
{
	prot::util::UserOfflineNotifyParam notify_param;
	notify_param.app_id = app_id;
	notify_param.user_id = user_id;
	notify_param.user_type = 0; // TODO:
	notify_param.login_id = login_id;
	notify_param.client_id = 0; // TODO:
	notify_param.register_id = 0; // TODO:

	com::SigHdrParam hdr_param;
	hdr_param.seq = seq;

	Buffer send_sig_buf = BuildUserOfflineNotify(notify_param, hdr_param);

	// Build MQ message
	Buffer send_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_BETWEEN_SERVICE,
		seq,
		m_service_type,
		m_instance_id,
		"", // exchange
		"", // routing key
		"", // user data
		"", // trace data
		""); // extend data

	return std::make_optional<MqMsgPair>(send_mq_buf, send_sig_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPairOpt MsgBuilder::BuildServicePongPair(
	const ServicePingPairOpt& ping_pair,
	const Buffer& mq_buf, 
	const Buffer& sig_buf)
{
	// Protocol message header
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	Buffer send_sig_buf = prot::util::BuildPongMsg(sig_hdr->seq,
		m_service_name, m_service_type, m_instance_id);

	// MQ message header
	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)DP(mq_buf);

	Buffer send_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_BETWEEN_SERVICE,
		mq_hdr->seq,
		m_service_type,
		m_instance_id,
		"", // exchange
		"", // routing key
		"", // user data
		"", // trace data
		""); // extend data

	return std::make_optional<MqMsgPair>(send_mq_buf, send_sig_buf);
}

}