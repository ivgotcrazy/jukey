#include "stream-msg-builder.h"
#include "stream.pb.h"
#include "protocol.h"
#include "sig-msg-builder.h"
#include "log/util-log.h"
#include "common/util-pb.h"

using namespace jukey::util;

namespace jukey::prot::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildPubStreamReq(const PubStreamReqParam& req_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(req_param.app_id);
	stream.set_user_id(req_param.user_id);
	stream.set_stream_id(STRM_ID(req_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(req_param.stream));
	stream.set_media_src_id(MSRC_ID(req_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(req_param.stream));

	jukey::prot::PublishStreamReq req;
	req.set_app_id(req_param.app_id);
	req.set_user_id(req_param.user_id);
	req.set_user_type(req_param.user_type);
	req.set_token(req_param.token);
	req.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build publish stream request:{}", PbMsgToJson(req));

	com::Buffer buf((uint32_t)(req.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_PUBLISH_STREAM_REQ, buf, req, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildUnpubStreamReq(const UnpubStreamReqParam& req_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(req_param.app_id);
	stream.set_user_id(req_param.user_id);
	stream.set_stream_id(STRM_ID(req_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(req_param.stream));
	stream.set_media_src_id(MSRC_ID(req_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(req_param.stream));

	jukey::prot::UnpublishStreamReq req;
	req.set_app_id(req_param.app_id);
	req.set_user_id(req_param.user_id);
	req.set_user_type(req_param.user_type);
	req.set_token(req_param.token);
	req.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build unpublish stream request:{}", PbMsgToJson(req));

	com::Buffer buf((uint32_t)(req.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_UNPUBLISH_STREAM_REQ, buf, req, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildPubStreamRsp(const PubStreamRspParam & rsp_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(rsp_param.app_id);
	stream.set_user_id(rsp_param.user_id);
	stream.set_stream_id(STRM_ID(rsp_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(rsp_param.stream));
	stream.set_media_src_id(MSRC_ID(rsp_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(rsp_param.stream));

	jukey::prot::PublishStreamRsp rsp;
	rsp.set_app_id(rsp_param.app_id);
	rsp.set_user_id(rsp_param.user_id);
	rsp.set_user_type(rsp_param.user_type);
	rsp.set_result((uint32_t)rsp_param.result);
	rsp.set_msg(rsp_param.msg);
	rsp.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build publish stream response:{}", PbMsgToJson(rsp));

	com::Buffer buf((uint32_t)(rsp.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_PUBLISH_STREAM_RSP, buf, rsp, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildUnpubStreamRsp(const UnpubStreamRspParam& rsp_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(rsp_param.app_id);
	stream.set_user_id(rsp_param.user_id);
	stream.set_stream_id(STRM_ID(rsp_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(rsp_param.stream));
	stream.set_media_src_id(MSRC_ID(rsp_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(rsp_param.stream));

	jukey::prot::UnpublishStreamRsp rsp;
	rsp.set_app_id(rsp_param.app_id);
	rsp.set_user_id(rsp_param.user_id);
	rsp.set_user_type(rsp_param.user_type);
	rsp.set_result((uint32_t)rsp_param.result);
	rsp.set_msg(rsp_param.msg);
	rsp.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build unpublish stream response:{}", PbMsgToJson(rsp));

	com::Buffer buf((uint32_t)(rsp.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_UNPUBLISH_STREAM_RSP, buf, rsp, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildSubStreamReq(const SubStreamReqParam& req_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(req_param.app_id);
	stream.set_user_id(req_param.user_id);
	stream.set_stream_id(STRM_ID(req_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(req_param.stream));
	stream.set_media_src_id(MSRC_ID(req_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(req_param.stream));

	jukey::prot::SubscribeStreamReq req;
	req.set_app_id(req_param.app_id);
	req.set_user_id(req_param.user_id);
	req.set_user_type(req_param.user_type);
	req.set_token(req_param.token);
	req.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build subscribe stream request:{}", PbMsgToJson(req));

	com::Buffer buf((uint32_t)(req.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_SUBSCRIBE_STREAM_REQ, buf, req, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildSubStreamRsp(const SubStreamRspParam& rsp_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(rsp_param.app_id);
	stream.set_user_id(rsp_param.user_id);
	stream.set_stream_id(STRM_ID(rsp_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(rsp_param.stream));
	stream.set_media_src_id(MSRC_ID(rsp_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(rsp_param.stream));

	jukey::prot::SubscribeStreamRsp rsp;
	rsp.set_app_id(rsp_param.app_id);
	rsp.set_user_id(rsp_param.user_id);
	rsp.set_user_type(rsp_param.user_type);
	rsp.set_stream_addr(rsp_param.stream_addr);
	rsp.set_result((uint32_t)rsp_param.result);
	rsp.set_msg(rsp_param.msg);
	rsp.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build subscribe stream response:{}", PbMsgToJson(rsp));

	com::Buffer buf((uint32_t)(rsp.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_SUBSCRIBE_STREAM_RSP, buf, rsp, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildUnsubStreamReq(const UnsubStreamReqParam& req_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(req_param.app_id);
	stream.set_user_id(req_param.user_id);
	stream.set_stream_id(STRM_ID(req_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(req_param.stream));
	stream.set_media_src_id(MSRC_ID(req_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(req_param.stream));

	jukey::prot::UnsubscribeStreamReq req;
	req.set_app_id(req_param.app_id);
	req.set_user_id(req_param.user_id);
	req.set_user_type(req_param.user_type);
	req.set_token(req_param.token);
	req.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build unsubscribe stream request:{}", PbMsgToJson(req));

	com::Buffer buf((uint32_t)(req.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_UNSUBSCRIBE_STREAM_REQ, buf, req, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildUnsubStreamRsp(const UnsubStreamRspParam& rsp_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(rsp_param.app_id);
	stream.set_user_id(rsp_param.user_id);
	stream.set_stream_id(STRM_ID(rsp_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(rsp_param.stream));
	stream.set_media_src_id(MSRC_ID(rsp_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(rsp_param.stream));

	jukey::prot::UnsubscribeStreamRsp rsp;
	rsp.set_app_id(rsp_param.app_id);
	rsp.set_user_id(rsp_param.user_id);
	rsp.set_user_type(rsp_param.user_type);
	rsp.set_result((uint32_t)rsp_param.result);
	rsp.set_msg(rsp_param.msg);
	rsp.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build unsubscribe stream response:{}", PbMsgToJson(rsp));

	com::Buffer buf((uint32_t)(rsp.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_UNSUBSCRIBE_STREAM_RSP, buf, rsp, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildLoginSendChannelNotify(
	const LoginSendChannelNotifyParam& notify_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(notify_param.app_id);
	stream.set_user_id(notify_param.user_id);
	stream.set_stream_id(STRM_ID(notify_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(notify_param.stream));
	stream.set_media_src_id(MSRC_ID(notify_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(notify_param.stream));

	jukey::prot::LoginSendChannelNotify notify;
	notify.set_app_id(notify_param.app_id);
	notify.set_user_id(notify_param.user_id);
	notify.set_user_type(0); // TODO:
	notify.set_stream_addr(notify_param.stream_addr);
	notify.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build login send channel notify:{}", PbMsgToJson(notify));

	com::Buffer buf((uint32_t)(notify.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_LOGIN_SEND_CHANNEL_NOTIFY, buf, notify, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildLoginSendChannelAck(const LoginSendChannelAckParam& ack_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(ack_param.app_id);
	stream.set_user_id(ack_param.user_id);
	stream.set_stream_id(STRM_ID(ack_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(ack_param.stream));
	stream.set_media_src_id(MSRC_ID(ack_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(ack_param.stream));

	jukey::prot::LoginSendChannelAck ack;
	ack.set_app_id(ack_param.app_id);
	ack.set_user_id(ack_param.user_id);
	ack.set_user_type(0); // TODO:
	ack.set_result(ack_param.result);
	ack.set_msg(ack_param.msg);
	ack.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build login send channel ack:{}", PbMsgToJson(ack));

	com::Buffer buf((uint32_t)(ack.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_LOGIN_SEND_CHANNEL_ACK, buf, ack, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildGetParentNodeReq(const GetParentNodeReqParam& req_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(req_param.stream.src.app_id);
	stream.set_user_id(req_param.stream.src.user_id);
	stream.set_stream_id(STRM_ID(req_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(req_param.stream));
	stream.set_media_src_id(MSRC_ID(req_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(req_param.stream));

	jukey::prot::GetParentNodeReq req;
	req.set_service_type(req_param.service_type);
	req.set_instance_id(req_param.instance_id);
	req.set_service_addr(req_param.service_addr);
	req.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build get parent node request:{}", PbMsgToJson(req));

	com::Buffer buf((uint32_t)(req.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_GET_PARENT_NODE_REQ, buf, req, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildGetParentNodeRsp(const GetParentNodeRspParam& rsp_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(rsp_param.stream.src.app_id);
	stream.set_user_id(rsp_param.stream.src.user_id);
	stream.set_stream_id(STRM_ID(rsp_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(rsp_param.stream));
	stream.set_media_src_id(MSRC_ID(rsp_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(rsp_param.stream));

	jukey::prot::GetParentNodeRsp rsp;
	rsp.mutable_stream()->CopyFrom(stream);
	
	for (auto& item : rsp_param.nodes) {
		jukey::prot::StreamNode node;

		node.set_service_type(item.service_type);
		node.set_instance_id(item.instance_id);
		node.set_service_addr(item.service_addr);

		rsp.add_nodes()->CopyFrom(node);
	}

	UTIL_INF("Build get parent node response:{}", PbMsgToJson(rsp));

	com::Buffer buf((uint32_t)(rsp.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_GET_PARENT_NODE_RSP, buf, rsp, hdr_param);

	return buf;
}

}