#include "msg-builder.h"
#include "protocol.h"
#include "stream-msg-builder.h"
#include "topo-msg-builder.h"
#include "mq-msg-builder.h"
#include "common/util-pb.h"

namespace
{

jukey::prot::MsgType GetRspMsgType(jukey::prot::MsgType req_msg_type)
{
	return req_msg_type == jukey::prot::MSG_MQ_FROM_ROUTE
		? jukey::prot::MSG_MQ_TO_ROUTE : jukey::prot::MSG_MQ_BETWEEN_SERVICE;
}

}

using namespace jukey::com;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MsgBuilder::MsgBuilder(uint32_t service_type, const std::string& instance_id)
	: m_service_type(service_type), m_instance_id(instance_id)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildServicePongPair(const Buffer& mq_buf,
	const Buffer& sig_buf, const ServiceConfig& config)
{
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	com::Buffer send_sig_buf = prot::util::BuildPongMsg(sig_hdr->seq,
		config.service_name,
		config.service_type,
		config.instance_id);

	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)DP(mq_buf);

	com::Buffer send_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_BETWEEN_SERVICE,
		mq_hdr->seq,
		config.service_type,
		config.instance_id,
		config.exchange,
		"", // routing key
		"", // user data
		"", // trace data
		""); // extend data

	return MqMsgPair(send_mq_buf, send_sig_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildSubStreamRspPair(const Buffer& mq_buf, 
	const Buffer& sig_buf,
	const SubStreamReqPairOpt& pair,
	ErrCode result,
	const std::string& msg)
{
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	prot::util::SubStreamRspParam rsp_param;
	rsp_param.app_id = pair->second.app_id();
	rsp_param.user_id = pair->second.user_id();
	rsp_param.user_type = pair->second.user_type();
	rsp_param.stream_addr = "UDP:192.168.28.201:5555";
	rsp_param.result = result;
	rsp_param.msg = msg;
	rsp_param.stream = util::ToMediaStream(pair->second.stream());

	com::SigHdrParam hdr_param;
	hdr_param.app_id = pair->second.app_id();
	hdr_param.user_id = pair->second.user_id();
	hdr_param.client_id = sig_hdr->clt;
	hdr_param.seq = sig_hdr->seq;

	com::Buffer rsp_prot_buf = 
		prot::util::BuildSubStreamRsp(rsp_param, hdr_param);

	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)DP(mq_buf);

	com::Buffer rsp_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_TO_ROUTE,
		mq_hdr->seq,
		m_service_type,
		m_instance_id,
		"", // exchange
		"", // routing key
		pair->first.user_data(),
		"", // trace data
		""); // extend data

	return MqMsgPair(rsp_mq_buf, rsp_prot_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildUnsubStreamRspPair(const Buffer& mq_buf,
	const Buffer& sig_buf,
	const UnsubStreamReqPairOpt& pair,
	com::ErrCode result,
	const std::string& msg)
{
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	prot::util::UnsubStreamRspParam rsp_param;
	rsp_param.app_id = pair->second.app_id();
	rsp_param.user_id = pair->second.user_id();
	rsp_param.user_type = pair->second.user_type();
	rsp_param.result = result;
	rsp_param.msg = msg;
	rsp_param.stream = util::ToMediaStream(pair->second.stream());

	com::SigHdrParam hdr_param;
	hdr_param.app_id = pair->second.app_id();
	hdr_param.user_id = pair->second.user_id();
	hdr_param.client_id = sig_hdr->clt;
	hdr_param.seq = sig_hdr->seq;

	com::Buffer rsp_prot_buf = 
		prot::util::BuildUnsubStreamRsp(rsp_param, hdr_param);

	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)DP(mq_buf);

	com::Buffer rsp_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_TO_ROUTE,
		mq_hdr->seq,
		m_service_type,
		m_instance_id,
		"", // exchange
		"", // routing key
		pair->first.user_data(),
		"", // trace data
		""); // extend data

	return MqMsgPair(rsp_mq_buf, rsp_prot_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildPubStreamRspPair(const com::Buffer& mq_buf, 
	const com::Buffer& sig_buf,
	const PubStreamReqPairOpt& req_pair,
	com::ErrCode result,
	const std::string& msg)
{
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	prot::util::PubStreamRspParam rsp_param;
	rsp_param.app_id = req_pair->second.app_id();
	rsp_param.user_id = req_pair->second.user_id();
	rsp_param.user_type = req_pair->second.user_type();
	rsp_param.result = result;
	rsp_param.msg = msg;
	rsp_param.stream = util::ToMediaStream(req_pair->second.stream());

	com::SigHdrParam hdr_param;
	hdr_param.app_id = req_pair->second.app_id();
	hdr_param.user_id = req_pair->second.user_id();
	hdr_param.seq = sig_hdr->seq;

	com::Buffer rsp_prot_buf = 
		prot::util::BuildPubStreamRsp(rsp_param, hdr_param);

	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)DP(mq_buf);

	com::Buffer rsp_mq_buf = prot::util::BuildMqMsg(
		GetRspMsgType((prot::MsgType)mq_hdr->mt),
		mq_hdr->seq,
		m_service_type,
		m_instance_id,
		"", // exchange
		"", // routing key
		req_pair->first.user_data(),
		"", // trace data
		""); // extend data

	return MqMsgPair(rsp_mq_buf, rsp_prot_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildUnpubStreamRspPair(const Buffer& mq_buf,
	const Buffer& sig_buf,
	const UnpubStreamReqPairOpt& req_pair,
	com::ErrCode result,
	const std::string& msg)
{
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	prot::util::UnpubStreamRspParam rsp_param;
	rsp_param.app_id = req_pair->second.app_id();
	rsp_param.user_id = req_pair->second.user_id();
	rsp_param.user_type = req_pair->second.user_type();
	rsp_param.result = result;
	rsp_param.msg = msg;
	rsp_param.stream = util::ToMediaStream(req_pair->second.stream());

	com::SigHdrParam hdr_param;
	hdr_param.app_id = req_pair->second.app_id();
	hdr_param.user_id = req_pair->second.user_id();
	hdr_param.seq = sig_hdr->seq;

	com::Buffer rsp_prot_buf = 
		prot::util::BuildUnpubStreamRsp(rsp_param, hdr_param);

	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)DP(mq_buf);

	com::Buffer rsp_mq_buf = prot::util::BuildMqMsg(
		GetRspMsgType((prot::MsgType)mq_hdr->mt),
		mq_hdr->seq,
		m_service_type,
		m_instance_id,
		"", // exchange
		"", // routing key
		req_pair->first.user_data(),
		"", // trace data
		""); // extend data

	return MqMsgPair(rsp_mq_buf, rsp_prot_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildLoginSendChannelNotifyPair(uint32_t seq,
	uint32_t app_id, 
	uint32_t user_id, 
	const com::MediaStream& stream,
	const std::string& addr)
{
	prot::util::LoginSendChannelNotifyParam notify_param;
	notify_param.app_id = app_id;
	notify_param.user_id = user_id;
	notify_param.stream = stream;
	notify_param.stream_addr = addr;

	com::SigHdrParam hdr_param;
	hdr_param.app_id = app_id;
	hdr_param.user_id = user_id;
	hdr_param.seq = seq;

	com::Buffer send_sig_buf = prot::util::BuildLoginSendChannelNotify(
		notify_param, hdr_param);

	com::Buffer send_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_TO_ROUTE,
		0,
		m_service_type,
		m_instance_id,
		"route-exchange",
		"", // routing key
		"", // user data
		"", // trace data
		""); // extend data

	return MqMsgPair(send_mq_buf, send_sig_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildLoginSendChannelAckPair(const Buffer& mq_buf,
	const Buffer& sig_buf, 
	const prot::LoginSendChannelNotify& notify,
	uint32_t result,
	const std::string& msg)
{
	prot::util::LoginSendChannelAckParam ack_param;
	ack_param.app_id = notify.app_id();
	ack_param.user_id = notify.user_id();
	ack_param.result = result;
	ack_param.msg = msg;
	ack_param.stream = util::ToMediaStream(notify.stream());

	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	com::SigHdrParam hdr_param;
	hdr_param.app_id = notify.app_id();
	hdr_param.user_id = notify.user_id();
	hdr_param.seq = sig_hdr->seq;

	com::Buffer send_sig_buf = prot::util::BuildLoginSendChannelAck(ack_param, 
		hdr_param);

	com::Buffer send_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_BETWEEN_SERVICE,
		0,
		m_service_type,
		m_instance_id,
		"", // exchange
		"", // routing key
		"", // user data
		"", // trace data
		""); // extend data

	return MqMsgPair(send_mq_buf, send_sig_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildGetParentNodeRspPair(const Buffer& mq_buf,
	const Buffer& sig_buf,
	const prot::GetParentNodeReq& req)
{
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	prot::util::GetParentNodeRspParam rsp_param;
	rsp_param.stream = util::ToMediaStream(req.stream());

	// TODO: 对于单机模式，这里直接返回请求者的信息
	prot::util::StreamNodeEntry node;
	node.service_type = req.service_type();
	node.instance_id = req.instance_id();
	node.service_addr = req.service_addr();
	rsp_param.nodes.push_back(node);

	com::SigHdrParam hdr_param;
	hdr_param.seq = sig_hdr->seq;

	com::Buffer rsp_prot_buf = prot::util::BuildGetParentNodeRsp(rsp_param,
		hdr_param);

	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)DP(mq_buf);

	com::Buffer rsp_mq_buf = prot::util::BuildMqMsg(
		GetRspMsgType((prot::MsgType)mq_hdr->mt),
		mq_hdr->seq,
		m_service_type,
		m_instance_id,
		"", // exchange
		"", // routing key
		"", // user data
		"", // trace data
		""); // extend data

	return MqMsgPair(rsp_mq_buf, rsp_prot_buf);
}

}