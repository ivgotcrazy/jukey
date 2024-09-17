#include "msg-builder.h"
#include "protocol.h"
#include "group-msg-builder.h"
#include "stream-msg-builder.h"
#include "topo-msg-builder.h"
#include "mq-msg-builder.h"
#include "log.h"
#include "common/util-pb.h"

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
MqMsgPair MsgBuilder::BuildJoinGroupRspPair(const com::Buffer& mq_buf,
	const com::Buffer& sig_buf,
	JoinGroupReqPairOpt req_pair,
	std::map<uint32_t, UserEntry> users,
	com::ErrCode result,
	const std::string& msg)
{
	prot::util::JoinGroupRspParam rsp_param;
	rsp_param.app_id = req_pair->second.app_id();
	rsp_param.group_id = req_pair->second.group_id();
	rsp_param.user_id = req_pair->second.user_id();
	rsp_param.user_type = req_pair->second.user_type();
	rsp_param.result = result;
	rsp_param.msg = msg;
	
	for (const auto& item : users) {
		prot::util::GroupUserEntry group_user_entry;
		group_user_entry.user_id = item.second.user_id;
		group_user_entry.user_type = item.second.user_type;

		for (const auto& media : item.second.medias) {
			prot::util::MediaStateEntry state_entry;
			state_entry.media_entry.media_src_id = media.media_src_id;
			state_entry.media_entry.media_src_type = (uint32_t)media.media_src_type;
			state_entry.media_entry.stream_id = media.stream_id;
			state_entry.media_entry.stream_type = (uint32_t)media.stream_type;
			state_entry.media_state = media.state;
			group_user_entry.media_state_entries.push_back(state_entry);
		}

		rsp_param.group_users.push_back(group_user_entry);
	}

	// Protocol message header
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	com::SigHdrParam hdr_param;
	hdr_param.app_id = req_pair->second.app_id();
	hdr_param.group_id = req_pair->second.group_id();
	hdr_param.user_id = req_pair->second.user_id();
	hdr_param.client_id = sig_hdr->clt;
	hdr_param.seq = sig_hdr->seq;

	// Build response
	Buffer send_prot_buf = BuildJoinGroupRsp(rsp_param, hdr_param);

	// MQ message header
	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)(mq_buf.data.get());

	// Build MQ message
	Buffer send_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_TO_ROUTE,
		mq_hdr->seq,
		m_service_type,
		m_instance_id,
		"", // exchange
		"", // routing-key
		req_pair->first.user_data(),
		"", // trace data
		""); // extend data

	return MqMsgPair(send_mq_buf, send_prot_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildLeaveGroupRspPair(const Buffer& mq_buf,
	const com::Buffer& sig_buf,
	LeaveGroupReqPairOpt req_pair,
	com::ErrCode result,
	const std::string& msg)
{
	prot::util::LeaveGroupRspParam rsp_param;
	rsp_param.app_id = req_pair->second.app_id();
	rsp_param.group_id = req_pair->second.group_id();
	rsp_param.user_id = req_pair->second.user_id();
	rsp_param.user_type = req_pair->second.user_type();
	rsp_param.result = result;
	rsp_param.msg = msg;

	// Protocol message header
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	com::SigHdrParam hdr_param;
	hdr_param.app_id = req_pair->second.app_id();
	hdr_param.group_id = req_pair->second.group_id();
	hdr_param.user_id = req_pair->second.user_id();
	hdr_param.client_id = sig_hdr->clt;
	hdr_param.seq = sig_hdr->seq;

	// Build response
	Buffer send_prot_buf = BuildLeaveGroupRsp(rsp_param, hdr_param);

	// MQ message header
	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)(mq_buf.data.get());

	// Build MQ message
	Buffer send_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_TO_ROUTE,
		mq_hdr->seq,
		m_service_type,
		m_instance_id,
		"", // exchange
		"", // routing-key
		req_pair->first.user_data(),
		"", // trace data
		""); // extend data

	return MqMsgPair(send_mq_buf, send_prot_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildJoinGroupNotifyPair(const com::Buffer& mq_buf,
	const com::Buffer& sig_buf,
	JoinGroupReqPairOpt req_pair,
	const UserEntry& user_entry)
{
	// Protocol message header
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	prot::util::JoinGroupNotifyParam notify_param;
	notify_param.app_id = req_pair->second.app_id();
	notify_param.group_id = req_pair->second.group_id();
	notify_param.user_id = req_pair->second.user_id();
	notify_param.user_type = req_pair->second.user_type();

	for (const auto& item : user_entry.medias) {
		prot::util::MediaEntry entry;
		entry.media_src_id   = item.media_src_id;
		entry.media_src_type = (uint32_t)item.media_src_type;
		entry.stream_id      = item.stream_id;
		entry.stream_type    = (uint32_t)item.stream_type;

		notify_param.media_entries.push_back(entry);
	}

	com::SigHdrParam hdr_param;
	hdr_param.app_id = req_pair->second.app_id();
	hdr_param.group_id = req_pair->second.group_id();
	hdr_param.user_id = 0;
	hdr_param.client_id = 0; // TODO:
	hdr_param.seq = 0; // TODO:

	// Build response
	Buffer send_prot_buf = BuildJoinGroupNotify(notify_param, hdr_param);

	// MQ message header
	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)(mq_buf.data.get());

	// Build MQ message
	Buffer send_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_TO_ROUTE,
		mq_hdr->seq,
		m_service_type,
		m_instance_id,
		"", // exchange
		"", // routing-key
		"", // user data
		"", // trace data
		""); // extend data

	return MqMsgPair(send_mq_buf, send_prot_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildLeaveGroupNotifyPair(const Buffer& mq_buf,
	const Buffer& sig_buf, LeaveGroupReqPairOpt req_pair)
{
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	prot::util::LeaveGroupNotifyParam notify_param;
	notify_param.app_id = req_pair->second.app_id();
	notify_param.group_id = req_pair->second.group_id();
	notify_param.user_id = req_pair->second.user_id();
	notify_param.user_type = req_pair->second.user_type();

	com::SigHdrParam hdr_param;
	hdr_param.app_id = req_pair->second.app_id();
	hdr_param.group_id = req_pair->second.group_id();
	hdr_param.user_id = 0;
	hdr_param.client_id = 0; // TODO:
	hdr_param.seq = 0; // TODO:

	Buffer send_prot_buf = BuildLeaveGroupNotify(notify_param, hdr_param);

	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)(mq_buf.data.get());

	Buffer send_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_TO_ROUTE,
		mq_hdr->seq,
		m_service_type,
		m_instance_id,
		"", // exchange
		"", // routing-key
		"", // user data
		"", // trace data
		""); // extend data

	return MqMsgPair(send_mq_buf, send_prot_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildLeaveGroupNotifyPair(uint32_t app, uint32_t group, 
	uint32_t user, uint32_t ut, uint32_t seq)
{
	prot::util::LeaveGroupNotifyParam notify_param;
	notify_param.app_id = app;
	notify_param.group_id = group;
	notify_param.user_id = user;
	notify_param.user_type = ut;

	com::SigHdrParam hdr_param;
	hdr_param.app_id = app;
	hdr_param.group_id = group;
	hdr_param.user_id = 0;
	hdr_param.client_id = 0; // TODO:
	hdr_param.seq = 0; // TODO:

	Buffer send_prot_buf = BuildLeaveGroupNotify(notify_param, hdr_param);

	Buffer send_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_TO_ROUTE,
		seq,
		m_service_type,
		m_instance_id,
		"", // exchange
		"", // routing-key
		"", // user data
		"", // trace data
		""); // extend data

	return MqMsgPair(send_mq_buf, send_prot_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildPublishMediaRspPair(const com::Buffer& mq_buf, 
	const com::Buffer& sig_buf,
	const PubMediaReqPairOpt& req_pair,
	uint32_t result, 
	const std::string& msg)
{
	prot::util::MediaEntry media_entry;
	media_entry.media_src_id = req_pair->second.media_entry().media_src_id();
	media_entry.media_src_type = req_pair->second.media_entry().media_src_type();
	media_entry.stream_id = req_pair->second.media_entry().stream_id();
	media_entry.stream_type = req_pair->second.media_entry().stream_type();

	prot::util::PubMediaRspParam rsp_param;
	rsp_param.app_id = req_pair->second.app_id();
	rsp_param.group_id = req_pair->second.group_id();
	rsp_param.user_id = req_pair->second.user_id();
	rsp_param.user_type = req_pair->second.user_type();
	rsp_param.result = result;
	rsp_param.msg = msg;
	rsp_param.media_entry = media_entry;
	

	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	com::SigHdrParam hdr_param;
	hdr_param.app_id = sig_hdr->app;
	hdr_param.group_id = sig_hdr->grp;
	hdr_param.user_id = sig_hdr->usr;
	hdr_param.client_id = sig_hdr->clt;
	hdr_param.seq = sig_hdr->seq;

	Buffer send_sig_buf = prot::util::BuildPubMediaRsp(rsp_param, hdr_param);

	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)DP(mq_buf);

	Buffer send_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_TO_ROUTE,
		mq_hdr->seq,
		m_service_type,
		m_instance_id,
		"", // exchange
		"", // routing-key
		req_pair->first.user_data(),
		"", // trace data
		""); // extend data

	return MqMsgPair(send_mq_buf, send_sig_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildUnpublishMediaRspPair(const com::Buffer& mq_buf,
	const com::Buffer& sig_buf,
	const UnpubMediaReqPairOpt& req_pair,
	uint32_t result,
	const std::string& msg)
{
	prot::util::MediaEntry media_entry;
	media_entry.media_src_id = req_pair->second.media_entry().media_src_id();
	media_entry.media_src_type = req_pair->second.media_entry().media_src_type();
	media_entry.stream_id = req_pair->second.media_entry().stream_id();
	media_entry.stream_type = req_pair->second.media_entry().stream_type();

	prot::util::UnpubMediaRspParam rsp_param;
	rsp_param.app_id = req_pair->second.app_id();
	rsp_param.group_id = req_pair->second.group_id();
	rsp_param.user_id = req_pair->second.user_id();
	rsp_param.user_type = req_pair->second.user_type();
	rsp_param.result = result;
	rsp_param.msg = msg;
	rsp_param.media_entry = media_entry;

	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	com::SigHdrParam hdr_param;
	hdr_param.app_id = sig_hdr->app;
	hdr_param.group_id = sig_hdr->grp;
	hdr_param.user_id = sig_hdr->usr;
	hdr_param.client_id = sig_hdr->clt;
	hdr_param.seq = sig_hdr->seq;

	Buffer send_sig_buf = prot::util::BuildUnpubMediaRsp(rsp_param, hdr_param);

	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)DP(mq_buf);

	Buffer send_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_TO_ROUTE,
		mq_hdr->seq,
		m_service_type,
		m_instance_id,
		"", // exchange
		"", // routing-key
		req_pair->first.user_data(),
		"", // trace data
		""); // extend data

	return MqMsgPair(send_mq_buf, send_sig_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildPublishMediaNotifyPair(const com::Buffer& mq_buf,
	const com::Buffer& sig_buf,
	const prot::PublishMediaReq& req,
	bool need_ack,
	uint32_t seq)
{
	prot::util::MediaEntry media_entry;
	media_entry.media_src_id = req.media_entry().media_src_id();
	media_entry.media_src_type = req.media_entry().media_src_type();
	media_entry.stream_id = req.media_entry().stream_id();
	media_entry.stream_type = req.media_entry().stream_type();

	prot::util::PubMediaNotifyParam notify_param;
	notify_param.app_id = req.app_id();
	notify_param.group_id = req.group_id();
	notify_param.user_id = req.user_id();
	notify_param.user_type = req.user_type();
	notify_param.need_ack = need_ack;
	notify_param.media_entry = media_entry;

	com::SigHdrParam hdr_param;
	hdr_param.app_id = req.app_id();
	hdr_param.group_id = req.group_id();
	hdr_param.seq = seq;

	// Build notify
	Buffer notify_sig_buf = prot::util::BuildPubMediaNotify(notify_param, hdr_param);

	// Build MQ message
	Buffer notify_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_TO_ROUTE,
		0,
		m_service_type,
		m_instance_id,
		"", // exchange
		"", // routing key
		"", // user data
		"", // trace data
		""); // extend data

	return MqMsgPair(notify_mq_buf, notify_sig_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildUnpublishMediaNotifyPair(const com::Buffer& mq_buf,
	const com::Buffer& sig_buf,
	const prot::UnpublishMediaReq& req,
	bool need_ack,
	uint32_t seq)
{
	prot::util::MediaEntry media_entry;
	media_entry.media_src_id = req.media_entry().media_src_id();
	media_entry.media_src_type = req.media_entry().media_src_type();
	media_entry.stream_id = req.media_entry().stream_id();
	media_entry.stream_type = req.media_entry().stream_type();

	prot::util::UnpubMediaNotifyParam notify_param;
	notify_param.app_id = req.app_id();
	notify_param.group_id = req.group_id();
	notify_param.user_id = req.user_id();
	notify_param.user_type = req.user_type();
	notify_param.need_ack = need_ack;
	notify_param.media_entry = media_entry;

	com::SigHdrParam hdr_param;
	hdr_param.app_id = req.app_id();
	hdr_param.group_id = req.group_id();
	hdr_param.seq = seq;

	// Build notify
	Buffer notify_sig_buf = prot::util::BuildUnpubMediaNotify(notify_param, hdr_param);

	// Build MQ message
	Buffer notify_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_TO_ROUTE,
		0,
		m_service_type,
		m_instance_id,
		"", // exchange
		"", // routing key
		"", // user data
		"", // trace data
		""); // extend data

	return MqMsgPair(notify_mq_buf, notify_sig_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MqMsgPair MsgBuilder::BuildServicePongPair(const Buffer& mq_buf, 
	const Buffer& sig_buf,
	uint32_t service_type,
	const std::string& service_name, 
	const std::string& instance_id)
{
	// Protocol message header
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);

	com::Buffer send_sig_buf = prot::util::BuildPongMsg(
		sig_hdr->seq,
		service_name,
		service_type,
		instance_id);

	// MQ message header
	prot::MqMsgHdr* mq_hdr = (prot::MqMsgHdr*)DP(mq_buf);

	com::Buffer send_mq_buf = prot::util::BuildMqMsg(
		prot::MSG_MQ_BETWEEN_SERVICE,
		mq_hdr->seq,
		service_type,
		instance_id,
		"", // exchange
		"", // routing key
		"", // user data
		"", // trace data
		""); // extend data

	return MqMsgPair(send_mq_buf, send_sig_buf);
}

}