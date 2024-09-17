#include "msg-builder.h"
#include "protocol.h"
#include "terminal-msg-builder.h"
#include "mq-msg-builder.h"
#include "log.h"
#include "protoc/mq.pb.h"
#include "topo-msg-builder.h"

using namespace jukey::com;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MsgBuilder::MsgBuilder(const TerminalServiceConfig& config): m_config(config)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildClientOfflineRspPair(const Buffer& mq_buf,
	const Buffer& sig_buf, const prot::ClientOfflineReq& req)
{
	prot::util::ClientOfflineRspParam rsp_param;
	rsp_param.app_id = req.app_id();
	rsp_param.client_id = req.client_id();
	rsp_param.session_id = req.session_id();

	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	com::SigHdrParam hdr_param;
	hdr_param.app_id = req.app_id();
	hdr_param.client_id = req.client_id();
	hdr_param.seq = sig_hdr->seq;

	// Build client offline response
	com::Buffer rsp_sig_buf = prot::util::BuildClientOfflineRsp(rsp_param,
		hdr_param);

	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)(mq_buf.data.get());

	// Build MQ message
	com::Buffer rsp_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_TO_ROUTE,
		mq_hdr->seq,
		m_config.service_config.service_type,
		m_config.service_config.instance_id,
		"", // exchange
		"", // routing key
		"", // user data
		"", // trace data
		""); // extend data

	return std::pair(rsp_mq_buf, rsp_sig_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildClientOfflineNotifyPair(const Buffer& mq_buf,
	const Buffer& sig_buf, const prot::ClientOfflineReq& req, uint32_t register_id)
{
	prot::util::ClientOfflineNotifyParam notify_param;
	notify_param.app_id = req.app_id();
	notify_param.client_id = req.client_id();
	notify_param.register_id = register_id;

	com::SigHdrParam hdr_param;
	hdr_param.app_id = req.app_id();
	hdr_param.seq = 0; // TODO:

	// Build client offline notify
	com::Buffer notify_sig_buf = prot::util::BuildClientOfflineNotify(
		notify_param, hdr_param);

	// Build MQ message
	com::Buffer notify_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_BETWEEN_SERVICE,
		0, // TODO: seq
		m_config.service_config.service_type,
		m_config.service_config.instance_id,
		"", // exchange
		"", // routing key
		"", // user data
		"", // trace data
		""); // extend data

	return std::pair(notify_mq_buf, notify_sig_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildRegRsp(const Buffer& mq_buf, const Buffer& sig_buf,
	const prot::RegisterReq& req, uint32_t register_id, ErrCode result,
	const std::string& msg)
{
	prot::util::ClientRegRspParam rsp_param;
	rsp_param.app_id = req.app_id();
	rsp_param.client_id = req.client_id();
	rsp_param.register_id = register_id;
	rsp_param.result = result;
	rsp_param.msg = msg;

	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	com::SigHdrParam hdr_param;
	hdr_param.app_id = req.app_id();
	hdr_param.client_id = req.client_id();
	hdr_param.seq = sig_hdr->seq;

	// Build register response
	com::Buffer rsp_sig_buf = prot::util::BuildClientRegRsp(rsp_param, hdr_param);

	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)(mq_buf.data.get());

	// Build MQ message
	com::Buffer rsp_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_TO_ROUTE,
		mq_hdr->seq,
		m_config.service_config.service_type,
		m_config.service_config.instance_id,
		m_config.service_config.exchange,
		"",
		"",
		"", // trace data
		""); // extend data

	return std::pair(rsp_mq_buf, rsp_sig_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildUnregRsp(const Buffer& mq_buf, const Buffer& sig_buf,
	const prot::UnregisterReq& req, ErrCode result, const std::string& msg)
{
	prot::util::ClientUnregRspParam rsp_param;
	rsp_param.app_id = req.app_id();
	rsp_param.client_id = req.client_id();
	rsp_param.register_id = req.register_id();
	rsp_param.result = result;
	rsp_param.msg = msg;

	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	com::SigHdrParam hdr_param;
	hdr_param.app_id = req.app_id();
	hdr_param.client_id = req.client_id();
	hdr_param.seq = sig_hdr->seq;

	// Build register response
	com::Buffer rsp_sig_buf = prot::util::BuildClientUnregRsp(rsp_param, hdr_param);

	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)(mq_buf.data.get());

	// Build MQ message
	com::Buffer rsp_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_TO_ROUTE,
		mq_hdr->seq,
		m_config.service_config.service_type,
		m_config.service_config.instance_id,
		m_config.service_config.exchange,
		"",
		"",
		"", // trace data
		""); // extend data

	return std::pair(rsp_mq_buf, rsp_sig_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildServicePong(const Buffer& mq_buf,
	const Buffer& sig_buf)
{
	// Protocol message header
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	com::Buffer pong_sig_buf = prot::util::BuildPongMsg(
		sig_hdr->seq,
		m_config.service_config.service_name,
		m_config.service_config.service_type,
		m_config.service_config.instance_id);

	// MQ message header
	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)DP(mq_buf);

	com::Buffer pong_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_BETWEEN_SERVICE,
		mq_hdr->seq,
		m_config.service_config.service_type,
		m_config.service_config.instance_id,
		m_config.service_config.exchange,
		"", // routing key
		"", // user data
		"", // trace data
		""); // extend data

	return std::pair(pong_mq_buf, pong_sig_buf);
}

}