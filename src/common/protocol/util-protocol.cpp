#include "util-protocol.h"
#include "protocol.h"
#include "log/util-log.h"

using namespace jukey::util;

namespace jukey::prot::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string MSG_TYPE_STR(uint32_t msg_type)
{
	switch (msg_type) {
	case MSG_MQ_FROM_ROUTE:
		return "MSG_MQ_FROM_ROUTE";
	case MSG_MQ_TO_ROUTE:
		return "MSG_MQ_TO_ROUTE";
	case MSG_MQ_BETWEEN_SERVICE:
		return "MSG_MQ_BETWEEN_SERVICE";
	case MSG_SERVICE_PING:
		return "MSG_SERVICE_PING";
	case MSG_SERVICE_PONG:
		return "MSG_SERVICE_PONG";
	case MSG_SYNC_ROUTE_ENTRIES:
		return "MSG_SYNC_ROUTE_ENTRIES";
	case MSG_CLIENT_REGISTER_REQ:
		return "MSG_CLIENT_REGISTER_REQ";
	case MSG_CLIENT_REGISTER_RSP:
		return "MSG_CLIENT_REGISTER_RSP";
	case MSG_CLIENT_UNREGISTER_REQ:
		return "MSG_CLIENT_UNREGISTER_REQ";
	case MSG_CLIENT_UNREGISTER_RSP:
		return "MSG_CLIENT_UNREGISTER_RSP";
	case MSG_CLIENT_OFFLINE_REQ:
		return "MSG_CLIENT_OFFLINE_REQ";
	case MSG_CLIENT_OFFLINE_RSP:
		return "MSG_CLIENT_OFFLINE_RSP";
	case MSG_CLIENT_OFFLINE_NOTIFY:
		return "MSG_CLIENT_OFFLINE_NOTIFY";
	case MSG_USER_LOGIN_REQ:
		return "MSG_USER_LOGIN_REQ";
	case MSG_USER_LOGIN_RSP:
		return "MSG_USER_LOGIN_RSP";
	case MSG_USER_LOGOUT_REQ:
		return "MSG_USER_LOGOUT_REQ";
	case MSG_USER_LOGOUT_RSP:
		return "MSG_USER_LOGOUT_RSP";
	case MSG_USER_OFFLINE_NOTIFY:
		return "MSG_USER_OFFLINE_NOTIFY";
	case MSG_JOIN_GROUP_REQ:
		return "MSG_JOIN_GROUP_REQ";
	case MSG_JOIN_GROUP_RSP:
		return "MSG_JOIN_GROUP_RSP";
	case MSG_JOIN_GROUP_NOTIFY:
		return "MSG_JOIN_GROUP_NOTIFY";
	case MSG_LEAVE_GROUP_REQ:
		return "MSG_LEAVE_GROUP_REQ";
	case MSG_LEAVE_GROUP_RSP:
		return "MSG_LEAVE_GROUP_RSP";
	case MSG_LEAVE_GROUP_NOTIFY:
		return "MSG_LEAVE_GROUP_NOTIFY";
	case MSG_PUBLISH_MEDIA_REQ:
		return "MSG_PUBLISH_MEDIA_REQ";
	case MSG_PUBLISH_MEDIA_RSP:
		return "MSG_PUBLISH_MEDIA_RSP";
	case MSG_PUBLISH_MEDIA_NOTIFY:
		return "MSG_PUBLISH_MEDIA_NOTIFY";
	case MSG_PUBLISH_MEDIA_ACK:
		return "MSG_PUBLISH_MEDIA_ACK";
	case MSG_UNPUBLISH_MEDIA_REQ:
		return "MSG_UNPUBLISH_MEDIA_REQ";
	case MSG_UNPUBLISH_MEDIA_RSP:
		return "MSG_UNPUBLISH_MEDIA_RSP";
	case MSG_UNPUBLISH_MEDIA_NOTIFY:
		return "MSG_UNPUBLISH_MEDIA_NOTIFY";
	case MSG_UNPUBLISH_MEDIA_ACK:
		return "MSG_UNPUBLISH_MEDIA_ACK";
	case MSG_PUBLISH_STREAM_REQ:
		return "MSG_PUBLISH_STREAM_REQ";
	case MSG_PUBLISH_STREAM_RSP:
		return "MSG_PUBLISH_STREAM_RSP";
	case MSG_UNPUBLISH_STREAM_REQ:
		return "MSG_UNPUBLISH_STREAM_REQ";
	case MSG_UNPUBLISH_STREAM_RSP:
		return "MSG_UNPUBLISH_STREAM_RSP";
	case MSG_SUBSCRIBE_STREAM_REQ:
		return "MSG_SUBSCRIBE_STREAM_REQ";
	case MSG_SUBSCRIBE_STREAM_RSP:
		return "MSG_SUBSCRIBE_STREAM_RSP";
	case MSG_UNSUBSCRIBE_STREAM_REQ:
		return "MSG_UNSUBSCRIBE_STREAM_REQ";
	case MSG_UNSUBSCRIBE_STREAM_RSP:
		return "MSG_UNSUBSCRIBE_STREAM_RSP";
	case MSG_GET_PARENT_NODE_REQ:
		return "MSG_GET_PARENT_NODE_REQ";
	case MSG_GET_PARENT_NODE_RSP:
		return "MSG_GET_PARENT_NODE_RSP";
	case MSG_LOGIN_SEND_CHANNEL_NOTIFY:
		return "MSG_LOGIN_SEND_CHANNEL_NOTIFY";
	case MSG_LOGIN_SEND_CHANNEL_ACK:
		return "MSG_LOGIN_SEND_CHANNEL_ACK";
	case MSG_LOGIN_SEND_CHANNEL_REQ:
		return "MSG_LOGIN_SEND_CHANNEL_REQ";
	case MSG_LOGIN_SEND_CHANNEL_RSP:
		return "MSG_LOGIN_SEND_CHANNEL_RSP";
	case MSG_LOGOUT_SEND_CHANNEL_REQ:
		return "MSG_LOGOUT_SEND_CHANNEL_REQ";
	case MSG_LOGOUT_SEND_CHANNEL_RSP:
		return "MSG_LOGOUT_SEND_CHANNEL_RSP";
	case MSG_LOGIN_RECV_CHANNEL_REQ:
		return "MSG_LOGIN_RECV_CHANNEL_REQ";
	case MSG_LOGIN_RECV_CHANNEL_RSP:
		return "MSG_LOGIN_RECV_CHANNEL_RSP";
	case MSG_LOGOUT_RECV_CHANNEL_REQ:
		return "MSG_LOGOUT_RECV_CHANNEL_REQ";
	case MSG_LOGOUT_RECV_CHANNEL_RSP:
		return "MSG_LOGOUT_RECV_CHANNEL_RSP";
	case MSG_NEGOTIATE_REQ:
		return "MSG_NEGOTIATE_REQ";
	case MSG_NEGOTIATE_RSP:
		return "MSG_NEGOTIATE_RSP";
	case MSG_START_SEND_STREAM_NOTIFY:
		return "MSG_START_SEND_STREAM_NOTIFY";
	case MSG_START_SEND_STREAM_ACK:
		return "MSG_START_SEND_STREAM_ACK";
	case MSG_STOP_SEND_STREAM_NOTIFY:
		return "MSG_STOP_SEND_STREAM_NOTIFY";
	case MSG_STOP_SEND_STREAM_ACK:
		return "MSG_STOP_SEND_STREAM_ACK";
	case MSG_PAUSE_RECV_STREAM_REQ:
		return "MSG_PAUSE_RECV_STREAM_REQ";
	case MSG_PAUSE_RECV_STREAM_RSP:
		return "MSG_PAUSE_RECV_STREAM_RSP";
	case MSG_RESUME_RECV_STREAM_REQ:
		return "MSG_RESUME_RECV_STREAM_REQ";
	case MSG_RESUME_RECV_STREAM_RSP:
		return "MSG_RESUME_RECV_STREAM_RSP";
	case MSG_STREAM_DATA:
		return "MSG_STREAM_DATA";
	case MSG_STREAM_FEEDBACK:
		return "MSG_STREAM_FEEDBACK";
	default:
		UTIL_ERR("Unknown message type:{}", msg_type);
		return "unknown";
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void DumpMqHeader(jukey::util::SpdlogWrapperSP logger, prot::MqMsgHdr* mq_hdr)
{
	if (logger && logger->GetLogger()) {
		SPDLOG_LOGGER_INFO(logger->GetLogger(),
			"mq header: [ver:{}, res:{}, mt:{}, len:{}, seq:{}]",
			(uint32_t)mq_hdr->ver,
			(uint32_t)mq_hdr->res,
			(uint32_t)mq_hdr->mt,
			mq_hdr->len,
			mq_hdr->seq);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void DumpSignalHeader(jukey::util::SpdlogWrapperSP logger, prot::SigMsgHdr* sig_hdr)
{
	if (logger && logger->GetLogger()) {
			SPDLOG_LOGGER_INFO(logger->GetLogger(),
				"signal header: [app:{}, clt:{}, usr:{}, grp:{}, ver:{}, c:{}, u:{}, "
				"g:{}, e:{}, mt:{}, len:{}, seq:{}]",
				sig_hdr->app,
				sig_hdr->clt,
				sig_hdr->usr,
				sig_hdr->grp,
				(uint32_t)sig_hdr->ver,
				(uint32_t)sig_hdr->c,
				(uint32_t)sig_hdr->u,
				(uint32_t)sig_hdr->g,
				(uint32_t)sig_hdr->e,
				(uint32_t)sig_hdr->mt,
				sig_hdr->len,
				sig_hdr->seq);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::MediaStream ToMediaStream(const prot::MediaEntry& entry)
{
	com::MediaStream stream;
	stream.src.src_type = (com::MediaSrcType)entry.media_src_type();
	stream.src.src_id = entry.media_src_id();
	stream.stream.stream_type = (com::StreamType)entry.stream_type();
	stream.stream.stream_id = entry.stream_id();

	return stream;
}

}