#include "group-msg-builder.h"
#include "group.pb.h"
#include "protocol.h"
#include "sig-msg-builder.h"
#include "common/util-pb.h"
#include "log/util-log.h"
#include "common/util-pb.h"

using namespace jukey::util;

namespace jukey::prot::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildJoinGroupReq(const JoinGroupReqParam& req_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::JoinGroupReq req;
	req.set_app_id(req_param.app_id);
	req.set_group_id(req_param.group_id);
	req.set_user_id(req_param.user_id);
	req.set_user_type(req_param.user_type);
	req.set_login_id(req_param.login_id);
	req.set_token(req_param.token);

	for (const auto& media : req_param.media_entries) {
		jukey::prot::MediaEntry entry;
		entry.set_media_src_id(media.media_src_id);
		entry.set_media_src_type(media.media_src_type);
		entry.set_stream_id(media.stream_id);
		entry.set_stream_type(media.stream_type);

		req.add_media_entries()->CopyFrom(entry);
	}

	UTIL_INF("Build join group reqeust:{}", PbMsgToJson(req));

	com::Buffer buf((uint32_t)(req.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_JOIN_GROUP_REQ, buf, req, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildJoinGroupRsp(const JoinGroupRspParam& rsp_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::JoinGroupRsp rsp;
	rsp.set_app_id(rsp_param.app_id);
	rsp.set_group_id(rsp_param.group_id);
	rsp.set_user_id(rsp_param.user_id);
	rsp.set_user_type(rsp_param.user_type);
	rsp.set_login_id(rsp_param.login_id);
	rsp.set_result(rsp_param.result);
	rsp.set_msg(rsp_param.msg);

	for (const auto& group_user : rsp_param.group_users) {
		jukey::prot::GroupUser user;
		user.set_user_id(group_user.user_id);
		user.set_user_type(group_user.user_type);

		for (const auto& media_state_entry : group_user.media_state_entries) {
			jukey::prot::MediaEntry media_entry;
			media_entry.set_media_src_id(media_state_entry.media_entry.media_src_id);
			media_entry.set_media_src_type(media_state_entry.media_entry.media_src_type);
			media_entry.set_stream_id(media_state_entry.media_entry.stream_id);
			media_entry.set_stream_type(media_state_entry.media_entry.stream_type);

			jukey::prot::MediaStateEntry state_entry;
			state_entry.set_media_state(media_state_entry.media_state);
			state_entry.mutable_media_entry()->CopyFrom(media_entry);

			user.add_media_state_entries()->CopyFrom(state_entry);
		}
		rsp.add_group_users()->CopyFrom(user);
	}

	UTIL_INF("Build join group response:{}", PbMsgToJson(rsp));

	com::Buffer buf((uint32_t)(rsp.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_JOIN_GROUP_RSP, buf, rsp, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildJoinGroupNotify(const JoinGroupNotifyParam& notify_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::JoinGroupNotify notify;
	notify.set_app_id(notify_param.app_id);
	notify.set_group_id(notify_param.group_id);
	notify.set_user_id(notify_param.user_id);
	notify.set_user_type(notify_param.user_type);

	for (const auto& media_entry : notify_param.media_entries) {
		jukey::prot::MediaEntry media;
		media.set_media_src_id(media_entry.media_src_id);
		media.set_media_src_type(media_entry.media_src_type);
		media.set_stream_id(media_entry.stream_id);
		media.set_stream_type(media_entry.stream_type);

		notify.add_media_entries()->CopyFrom(media);
	}

	UTIL_INF("Build join group notify:{}", PbMsgToJson(notify));

	com::Buffer buf((uint32_t)(notify.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_JOIN_GROUP_NOTIFY, buf, notify, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildLeaveGroupReq(const LeaveGroupReqParam& req_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::LeaveGroupReq req;
	req.set_app_id(req_param.app_id);
	req.set_group_id(req_param.group_id);
	req.set_user_id(req_param.user_id);
	req.set_user_type(req_param.user_type);
	req.set_login_id(req_param.login_id);
	req.set_token(req_param.token);

	UTIL_INF("Build leave group request:{}", PbMsgToJson(req));

	com::Buffer buf((uint32_t)(req.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_LEAVE_GROUP_REQ, buf, req, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildLeaveGroupRsp(const LeaveGroupRspParam& rsp_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::LeaveGroupRsp rsp;
	rsp.set_app_id(rsp_param.app_id);
	rsp.set_group_id(rsp_param.group_id);
	rsp.set_user_id(rsp_param.user_id);
	rsp.set_user_type(rsp_param.user_type);
	rsp.set_login_id(rsp_param.login_id);
	rsp.set_result(rsp_param.result);
	rsp.set_msg(rsp_param.msg);

	UTIL_INF("Build leave group response:{}", PbMsgToJson(rsp));

	com::Buffer buf((uint32_t)(rsp.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_LEAVE_GROUP_RSP, buf, rsp, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildLeaveGroupNotify(const LeaveGroupNotifyParam& notify_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::LeaveGroupNotify notify;
	notify.set_app_id(notify_param.app_id);
	notify.set_group_id(notify_param.group_id);
	notify.set_user_id(notify_param.user_id);
	notify.set_user_type(notify_param.user_type);

	UTIL_INF("Build leave group notify:{}", PbMsgToJson(notify));

	com::Buffer buf((uint32_t)(notify.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_LEAVE_GROUP_NOTIFY, buf, notify, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildPubMediaReq(const PubMediaReqParam& req_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::MediaEntry entry;
	entry.set_stream_id(req_param.media_entry.stream_id);
	entry.set_stream_type(req_param.media_entry.stream_type);
	entry.set_media_src_id(req_param.media_entry.media_src_id);
	entry.set_media_src_type(req_param.media_entry.media_src_type);

	jukey::prot::PublishMediaReq req;
	req.set_app_id(req_param.app_id);
	req.set_group_id(req_param.group_id);
	req.set_user_id(req_param.user_id);
	req.set_user_type(req_param.user_type);
	req.set_login_id(req_param.login_id);
	req.mutable_media_entry()->CopyFrom(entry);

	UTIL_INF("Build publish media request:{}", PbMsgToJson(req));

	com::Buffer buf((uint32_t)(req.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_PUBLISH_MEDIA_REQ, buf, req, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildUnpubMediaReq(const UnpubMediaReqParam& req_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::MediaEntry entry;
	entry.set_stream_id(req_param.media_entry.stream_id);
	entry.set_stream_type(req_param.media_entry.stream_type);
	entry.set_media_src_id(req_param.media_entry.media_src_id);
	entry.set_media_src_type(req_param.media_entry.media_src_type);

	jukey::prot::UnpublishMediaReq req;
	req.set_app_id(req_param.app_id);
	req.set_group_id(req_param.group_id);
	req.set_user_id(req_param.user_id);
	req.set_user_type(req_param.user_type);
	req.set_login_id(req_param.login_id);
	req.mutable_media_entry()->CopyFrom(entry);

	UTIL_INF("Build unpublish media request:{}", PbMsgToJson(req));

	com::Buffer buf((uint32_t)(req.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_UNPUBLISH_MEDIA_REQ, buf, req, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildPubMediaRsp(const PubMediaRspParam& rsp_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::MediaEntry entry;
	entry.set_stream_id(rsp_param.media_entry.stream_id);
	entry.set_stream_type(rsp_param.media_entry.stream_type);
	entry.set_media_src_id(rsp_param.media_entry.media_src_id);
	entry.set_media_src_type(rsp_param.media_entry.media_src_type);

	jukey::prot::PublishMediaRsp rsp;
	rsp.set_app_id(rsp_param.app_id);
	rsp.set_group_id(rsp_param.group_id);
	rsp.set_user_id(rsp_param.user_id);
	rsp.set_user_type(rsp_param.user_type);
	rsp.set_login_id(rsp_param.login_id);
	rsp.set_result(rsp_param.result);
	rsp.set_msg(rsp_param.msg);
	rsp.mutable_media_entry()->CopyFrom(entry);

	UTIL_INF("Build publish media response:{}", PbMsgToJson(rsp));

	com::Buffer buf((uint32_t)(rsp.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_PUBLISH_MEDIA_RSP, buf, rsp, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildUnpubMediaRsp(const UnpubMediaRspParam& rsp_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::MediaEntry entry;
	entry.set_stream_id(rsp_param.media_entry.stream_id);
	entry.set_stream_type(rsp_param.media_entry.stream_type);
	entry.set_media_src_id(rsp_param.media_entry.media_src_id);
	entry.set_media_src_type(rsp_param.media_entry.media_src_type);

	jukey::prot::UnpublishMediaRsp rsp;
	rsp.set_app_id(rsp_param.app_id);
	rsp.set_group_id(rsp_param.group_id);
	rsp.set_user_id(rsp_param.user_id);
	rsp.set_user_type(rsp_param.user_type);
	rsp.set_login_id(rsp_param.login_id);
	rsp.set_result(rsp_param.result);
	rsp.set_msg(rsp_param.msg);
	rsp.mutable_media_entry()->CopyFrom(entry);

	UTIL_INF("Build unpublish media response:{}", PbMsgToJson(rsp));

	com::Buffer buf((uint32_t)(rsp.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_UNPUBLISH_MEDIA_RSP, buf, rsp, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildPubMediaNotify(const PubMediaNotifyParam& notify_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::MediaEntry entry;
	entry.set_stream_id(notify_param.media_entry.stream_id);
	entry.set_stream_type(notify_param.media_entry.stream_type);
	entry.set_media_src_id(notify_param.media_entry.media_src_id);
	entry.set_media_src_type(notify_param.media_entry.media_src_type);

	jukey::prot::PublishMediaNotify notify;
	notify.set_app_id(notify_param.app_id);
	notify.set_group_id(notify_param.group_id);
	notify.set_user_id(notify_param.user_id);
	notify.set_user_type(notify_param.user_type);
	notify.set_need_ack(notify_param.need_ack);
	notify.mutable_media_entry()->CopyFrom(entry);

	UTIL_INF("Build publish media notify:{}", PbMsgToJson(notify));

	com::Buffer buf((uint32_t)(notify.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_PUBLISH_MEDIA_NOTIFY, buf, notify, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildUnpubMediaNotify(const UnpubMediaNotifyParam& notify_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::MediaEntry entry;
	entry.set_stream_id(notify_param.media_entry.stream_id);
	entry.set_stream_type(notify_param.media_entry.stream_type);
	entry.set_media_src_id(notify_param.media_entry.media_src_id);
	entry.set_media_src_type(notify_param.media_entry.media_src_type);

	jukey::prot::UnpublishMediaNotify notify;
	notify.set_app_id(notify_param.app_id);
	notify.set_group_id(notify_param.group_id);
	notify.set_user_id(notify_param.user_id);
	notify.set_user_type(notify_param.user_type);
	notify.set_need_ack(notify_param.need_ack);
	notify.mutable_media_entry()->CopyFrom(entry);

	UTIL_INF("Build unpublish media notify:{}", PbMsgToJson(notify));

	com::Buffer buf((uint32_t)(notify.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_UNPUBLISH_MEDIA_NOTIFY, buf, notify, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildPubMediaAck(const PubMediaAckParam& ack_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::MediaEntry entry;
	entry.set_stream_id(ack_param.media_entry.stream_id);
	entry.set_stream_type(ack_param.media_entry.stream_type);
	entry.set_media_src_id(ack_param.media_entry.media_src_id);
	entry.set_media_src_type(ack_param.media_entry.media_src_type);

	jukey::prot::PublishMediaAck ack;
	ack.set_app_id(ack_param.app_id);
	ack.set_group_id(ack_param.group_id);
	ack.set_user_id(ack_param.user_id);
	ack.set_ack_user_id(ack_param.ack_user_id);
	ack.set_ack_user_type(ack_param.ack_user_type);
	ack.mutable_media_entry()->CopyFrom(entry);

	UTIL_INF("Build publish media ack:{}", PbMsgToJson(ack));

	com::Buffer buf((uint32_t)(ack.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_PUBLISH_MEDIA_ACK, buf, ack, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildUnpubMediaAck(const UnpubMediaAckParam& ack_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::MediaEntry entry;
	entry.set_stream_id(ack_param.media_entry.stream_id);
	entry.set_stream_type(ack_param.media_entry.stream_type);
	entry.set_media_src_id(ack_param.media_entry.media_src_id);
	entry.set_media_src_type(ack_param.media_entry.media_src_type);

	jukey::prot::UnpublishMediaAck ack;
	ack.set_app_id(ack_param.app_id);
	ack.set_group_id(ack_param.group_id);
	ack.set_user_id(ack_param.user_id);
	ack.set_ack_user_id(ack_param.ack_user_id);
	ack.set_ack_user_type(ack_param.ack_user_type);
	ack.mutable_media_entry()->CopyFrom(entry);

	UTIL_INF("Build unpublish media ack:{}", PbMsgToJson(ack));

	com::Buffer buf((uint32_t)(ack.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_UNPUBLISH_MEDIA_ACK, buf, ack, hdr_param);

	return buf;
}

}