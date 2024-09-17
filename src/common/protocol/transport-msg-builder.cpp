#include "transport-msg-builder.h"
#include "transport.pb.h"
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
com::Buffer BuildLoginSendChannelReq(const LoginSendChannelReqParam& req_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(req_param.app_id);
	stream.set_user_id(req_param.user_id);
	stream.set_stream_id(STRM_ID(req_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(req_param.stream));
	stream.set_media_src_id(MSRC_ID(req_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(req_param.stream));

	jukey::prot::LoginSendChannelReq req;
	req.set_app_id(req_param.app_id);
	req.set_user_id(req_param.user_id);
	req.set_user_type(req_param.user_type);
	req.set_token(req_param.token);
	req.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build login send channel request:{}", PbMsgToJson(req));

	com::Buffer buf((uint32_t)(req.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_LOGIN_SEND_CHANNEL_REQ, buf, req, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildLoginSendChannelRsp(const LoginSendChannelRspParam& rsp_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(rsp_param.app_id);
	stream.set_user_id(rsp_param.user_id);
	stream.set_stream_id(STRM_ID(rsp_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(rsp_param.stream));
	stream.set_media_src_id(MSRC_ID(rsp_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(rsp_param.stream));

	jukey::prot::LoginSendChannelRsp rsp;
	rsp.set_app_id(rsp_param.app_id);
	rsp.set_user_id(rsp_param.user_id);
	rsp.set_user_type(rsp_param.user_type);
	rsp.set_channel_id(rsp_param.channel_id);
	rsp.set_result((uint32_t)rsp_param.result);
	rsp.set_msg(rsp_param.msg);
	rsp.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build login send channel response:{}", PbMsgToJson(rsp));

	com::Buffer buf((uint32_t)(rsp.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_LOGIN_SEND_CHANNEL_RSP, buf, rsp, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildLogoutSendChannelReq(const LogoutSendChannelReqParam& req_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(req_param.app_id);
	stream.set_user_id(req_param.user_id);
	stream.set_stream_id(STRM_ID(req_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(req_param.stream));
	stream.set_media_src_id(MSRC_ID(req_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(req_param.stream));

	jukey::prot::LogoutSendChannelReq req;
	req.set_app_id(req_param.app_id);
	req.set_user_id(req_param.user_id);
	req.set_user_type(req_param.user_type);
	req.set_channel_id(req_param.channe_id);
	req.set_token(req_param.token);
	req.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build logout send channel request:{}", PbMsgToJson(req));

	com::Buffer buf((uint32_t)(req.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_LOGOUT_SEND_CHANNEL_REQ, buf, req, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildLogoutSendChannelRsp(const LogoutSendChannelRspParam& rsp_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(rsp_param.app_id);
	stream.set_user_id(rsp_param.user_id);
	stream.set_stream_id(STRM_ID(rsp_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(rsp_param.stream));
	stream.set_media_src_id(MSRC_ID(rsp_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(rsp_param.stream));

	jukey::prot::LogoutSendChannelRsp rsp;
	rsp.set_app_id(rsp_param.app_id);
	rsp.set_user_id(rsp_param.user_id);
	rsp.set_user_type(rsp_param.user_type);
	rsp.set_channel_id(rsp_param.channel_id);
	rsp.set_result((uint32_t)rsp_param.result);
	rsp.set_msg(rsp_param.msg);
	rsp.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build logout send channel response:{}", PbMsgToJson(rsp));

	com::Buffer buf((uint32_t)(rsp.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_LOGOUT_SEND_CHANNEL_RSP, buf, rsp, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildLoginRecvChannelReq(const LoginRecvChannelReqParam& req_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(req_param.app_id);
	stream.set_user_id(req_param.user_id);
	stream.set_stream_id(STRM_ID(req_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(req_param.stream));
	stream.set_media_src_id(MSRC_ID(req_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(req_param.stream));

	jukey::prot::LoginRecvChannelReq req;
	req.set_app_id(req_param.app_id);
	req.set_user_id(req_param.user_id);
	req.set_user_type(req_param.user_type);
	req.set_token(req_param.token);
	req.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build logout recv channel request:{}", PbMsgToJson(req));

	com::Buffer buf((uint32_t)(req.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_LOGIN_RECV_CHANNEL_REQ, buf, req, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildLoginRecvChannelRsp(const LoginRecvChannelRspParam& rsp_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(rsp_param.app_id);
	stream.set_user_id(rsp_param.user_id);
	stream.set_stream_id(STRM_ID(rsp_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(rsp_param.stream));
	stream.set_media_src_id(MSRC_ID(rsp_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(rsp_param.stream));

	jukey::prot::LoginRecvChannelRsp rsp;
	rsp.set_app_id(rsp_param.app_id);
	rsp.set_user_id(rsp_param.user_id);
	rsp.set_user_type(rsp_param.user_type);
	rsp.set_channel_id(rsp_param.channel_id);
	rsp.set_result((uint32_t)rsp_param.result);
	rsp.set_msg(rsp_param.msg);
	rsp.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build login recv channel response:{}", PbMsgToJson(rsp));

	com::Buffer buf((uint32_t)(rsp.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_LOGIN_RECV_CHANNEL_RSP, buf, rsp, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildLogoutRecvChannelReq(const LogoutRecvChannelReqParam& req_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(req_param.app_id);
	stream.set_user_id(req_param.user_id);
	stream.set_stream_id(STRM_ID(req_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(req_param.stream));
	stream.set_media_src_id(MSRC_ID(req_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(req_param.stream));

	jukey::prot::LogoutRecvChannelReq req;
	req.set_app_id(req_param.app_id);
	req.set_user_id(req_param.user_id);
	req.set_user_type(req_param.user_type);
	req.set_channel_id(req_param.channel_id);
	req.set_token(req_param.token);
	req.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build logout recv channel request:{}", PbMsgToJson(req));

	com::Buffer buf((uint32_t)(req.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_LOGOUT_RECV_CHANNEL_REQ, buf, req, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildLogoutRecvChannelRsp(const LogoutRecvChannelRspParam& rsp_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(rsp_param.app_id);
	stream.set_user_id(rsp_param.user_id);
	stream.set_stream_id(STRM_ID(rsp_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(rsp_param.stream));
	stream.set_media_src_id(MSRC_ID(rsp_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(rsp_param.stream));

	jukey::prot::LogoutRecvChannelRsp rsp;
	rsp.set_app_id(rsp_param.app_id);
	rsp.set_user_id(rsp_param.user_id);
	rsp.set_user_type(rsp_param.user_type);
	rsp.set_channel_id(rsp_param.channel_id);
	rsp.set_result((uint32_t)rsp_param.result);
	rsp.set_msg(rsp_param.msg);
	rsp.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build logout recv channel response:{}", PbMsgToJson(rsp));

	com::Buffer buf((uint32_t)(rsp.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_LOGOUT_RECV_CHANNEL_RSP, buf, rsp, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer 
BuildStartSendStreamNotify(const StartSendStreamNotifyParam& notify_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(notify_param.stream.src.app_id);
	stream.set_user_id(notify_param.stream.src.user_id);
	stream.set_stream_id(STRM_ID(notify_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(notify_param.stream));
	stream.set_media_src_id(MSRC_ID(notify_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(notify_param.stream));

	jukey::prot::StartSendStreamNotify notify;
	notify.set_channel_id(notify_param.channel_id);
	notify.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build start send stream notify:{}", PbMsgToJson(notify));

	com::Buffer buf((uint32_t)(notify.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_START_SEND_STREAM_NOTIFY, buf, notify, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildStartSendStreamAck(const StartSendStreamAckParam& ack_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(ack_param.stream.src.app_id);
	stream.set_user_id(ack_param.stream.src.user_id);
	stream.set_stream_id(STRM_ID(ack_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(ack_param.stream));
	stream.set_media_src_id(MSRC_ID(ack_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(ack_param.stream));

	jukey::prot::StartSendStreamAck ack;
	ack.set_channel_id(ack_param.channel_id);
	ack.set_result(ack_param.result);
	ack.set_msg(ack_param.msg);
	ack.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build start send stream ack:{}", PbMsgToJson(ack));

	com::Buffer buf((uint32_t)(ack.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_START_SEND_STREAM_ACK, buf, ack, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildNegotiateReq(const NegotiateReqParam& req_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(req_param.stream.src.app_id);
	stream.set_user_id(req_param.stream.src.user_id);
	stream.set_stream_id(STRM_ID(req_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(req_param.stream));
	stream.set_media_src_id(MSRC_ID(req_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(req_param.stream));

	jukey::prot::NegotiateReq req;
	req.set_channel_id(req_param.channel_id);
	req.mutable_stream()->CopyFrom(stream);

	for (const auto& cap : req_param.caps) {
		req.add_caps(cap);
	}

	UTIL_INF("Build negotiate request:{}", PbMsgToJson(req));

	com::Buffer buf((uint32_t)(req.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_NEGOTIATE_REQ, buf, req, hdr_param);

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer BuildNegotiateRsp(const NegotiateRspParam& rsp_param,
	const com::SigHdrParam& hdr_param)
{
	jukey::prot::NetStream stream;
	stream.set_app_id(rsp_param.stream.src.app_id);
	stream.set_user_id(rsp_param.stream.src.user_id);
	stream.set_stream_id(STRM_ID(rsp_param.stream));
	stream.set_stream_type((uint32_t)STRM_TYPE(rsp_param.stream));
	stream.set_media_src_id(MSRC_ID(rsp_param.stream));
	stream.set_media_src_type((uint32_t)MSRC_TYPE(rsp_param.stream));

	jukey::prot::NegotiateRsp rsp;
	rsp.set_channel_id(rsp_param.channel_id);
	rsp.set_cap(rsp_param.cap);
	rsp.set_result(rsp_param.result);
	rsp.set_msg(rsp_param.msg);
	rsp.mutable_stream()->CopyFrom(stream);

	UTIL_INF("Build negotiate response:{}", PbMsgToJson(rsp));

	com::Buffer buf((uint32_t)(rsp.ByteSizeLong() + sizeof(SigMsgHdr)));

	ConstructSigMsg(MSG_NEGOTIATE_RSP, buf, rsp, hdr_param);

	return buf;
}

}