#include "msg-builder.h"
#include "protocol.h"
#include "stream-msg-builder.h"
#include "group-msg-builder.h"
#include "user-msg-builder.h"
#include "terminal-msg-builder.h"
#include "common/util-pb.h"


using namespace jukey::com;

namespace jukey::sdk
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MsgBuilder::MsgBuilder(const RtcEngineParam& param, const RtcEngineData& data)
	: m_engine_param(param), m_engine_data(data)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Buffer MsgBuilder::BuildPubStreamReq(const MediaStream& stream, uint32_t seq)
{
	prot::util::PubStreamReqParam req_param;
	req_param.app_id    = m_engine_param.app_id;
	req_param.user_id   = m_engine_data.user_id;
	req_param.user_type = 0; // TODO:
	req_param.token     = "unknown";

	req_param.stream = stream;

	com::SigHdrParam hdr_param;
	hdr_param.app_id    = m_engine_param.app_id;
	hdr_param.user_id   = m_engine_data.user_id;
	hdr_param.client_id = m_engine_param.client_id;
	hdr_param.seq       = seq;

	return prot::util::BuildPubStreamReq(req_param, hdr_param);
}
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Buffer MsgBuilder::BuildUnpubStreamReq(const MediaStream& stream, uint32_t seq)
{
	prot::util::UnpubStreamReqParam req_param;
	req_param.app_id    = m_engine_param.app_id;
	req_param.user_id   = m_engine_data.user_id;
	req_param.user_type = 0; // TODO:
	req_param.token     = "unknown";

	req_param.stream = stream;

	com::SigHdrParam hdr_param;
	hdr_param.app_id    = m_engine_param.app_id;
	hdr_param.user_id   = m_engine_data.user_id;
	hdr_param.client_id = m_engine_param.client_id;
	hdr_param.seq       = seq;

	return prot::util::BuildUnpubStreamReq(req_param, hdr_param);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Buffer MsgBuilder::BuildUnpubMediaReq(const MediaStream& stream, uint32_t seq)
{
	prot::util::MediaEntry media_entry;
	media_entry.stream_id      = stream.stream.stream_id;
	media_entry.stream_type    = (uint32_t)stream.stream.stream_type;
	media_entry.media_src_id   = stream.src.src_id;
	media_entry.media_src_type = (uint32_t)stream.src.src_type;

	prot::util::UnpubMediaReqParam req_param;
	req_param.app_id      = m_engine_param.app_id;
	req_param.group_id    = m_engine_data.group_id;
	req_param.user_id     = m_engine_data.user_id;
	req_param.user_type   = 0;
	req_param.media_entry = media_entry;

	com::SigHdrParam hdr_param;
	hdr_param.app_id    = m_engine_param.app_id;
	hdr_param.group_id  = m_engine_data.group_id;
	hdr_param.user_id   = m_engine_data.user_id;
	hdr_param.client_id = m_engine_param.client_id;
	hdr_param.seq       = seq;

	return prot::util::BuildUnpubMediaReq(req_param, hdr_param);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Buffer MsgBuilder::BuildPubMediaReq(const MediaStream& stream, uint32_t seq)
{
	prot::util::MediaEntry media_entry;
	media_entry.stream_id      = stream.stream.stream_id;
	media_entry.stream_type    = (uint32_t)stream.stream.stream_type;
	media_entry.media_src_id   = stream.src.src_id;
	media_entry.media_src_type = (uint32_t)stream.src.src_type;

	prot::util::PubMediaReqParam req_param;
	req_param.app_id      = m_engine_param.app_id;
	req_param.group_id    = m_engine_data.group_id;
	req_param.user_id     = m_engine_data.user_id;
	req_param.user_type   = 0;
	req_param.media_entry = media_entry;

	com::SigHdrParam hdr_param;
	hdr_param.app_id    = m_engine_param.app_id;
	hdr_param.group_id  = m_engine_data.group_id;
	hdr_param.user_id   = m_engine_data.user_id;
	hdr_param.client_id = m_engine_param.client_id;
	hdr_param.seq       = seq;

	return prot::util::BuildPubMediaReq(req_param, hdr_param);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Buffer MsgBuilder::BuildLoginSendChannelAck(const Buffer& buf,
	const prot::LoginSendChannelNotify& notify)
{
	prot::util::LoginSendChannelAckParam ack_param;
	ack_param.app_id  = notify.app_id();
	ack_param.user_id = notify.user_id();
	ack_param.stream  = util::ToMediaStream(notify.stream());
	ack_param.result  = ERR_CODE_OK; // TODO:
	ack_param.msg     = "success";

	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(buf);

	com::SigHdrParam hdr_param;
	hdr_param.app_id  = notify.app_id();
	hdr_param.user_id = notify.user_id();
	hdr_param.seq     = sig_hdr->seq;

	return prot::util::BuildLoginSendChannelAck(ack_param, hdr_param);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Buffer MsgBuilder::BuildSubStreamReq(const com::MediaStream& stream, uint32_t seq)
{
	prot::util::SubStreamReqParam req_param;
	req_param.app_id    = m_engine_param.app_id;
	req_param.user_id   = m_engine_data.user_id;
	req_param.user_type = 0; // TODO:
	req_param.stream    = stream;
	req_param.token     = "unknown";

	com::SigHdrParam hdr_param;
	hdr_param.app_id    = m_engine_param.app_id;
	hdr_param.user_id   = m_engine_data.user_id;
	hdr_param.client_id = m_engine_param.client_id;
	hdr_param.seq       = seq;

	return prot::util::BuildSubStreamReq(req_param, hdr_param);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Buffer MsgBuilder::BuildUnsubStreamReq(const com::MediaStream& stream, uint32_t seq)
{
	prot::util::UnsubStreamReqParam req_param;
	req_param.app_id = m_engine_param.app_id;
	req_param.user_id = m_engine_data.user_id;
	req_param.user_type = 0; // TODO:
	req_param.stream = stream;
	req_param.token = "unknown";

	com::SigHdrParam hdr_param;
	hdr_param.app_id = m_engine_param.app_id;
	hdr_param.user_id = m_engine_data.user_id;
	hdr_param.client_id = m_engine_param.client_id;
	hdr_param.seq = seq;

	return prot::util::BuildUnsubStreamReq(req_param, hdr_param);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Buffer MsgBuilder::BuildUserLoginReq(uint32_t user_id, uint32_t seq)
{
	prot::util::UserLoginReqParam req_param;
	req_param.app_id      = m_engine_param.app_id;
	req_param.client_id   = m_engine_param.client_id;
	req_param.register_id = m_engine_data.register_id;
	req_param.user_id     = user_id;
	req_param.user_type   = 0; // TODO:
	req_param.token       = "unknown";

	com::SigHdrParam hdr_param;
	hdr_param.client_id = m_engine_param.client_id;
	hdr_param.app_id    = m_engine_param.app_id;
	hdr_param.user_id   = user_id;
	hdr_param.seq       = seq;

	return prot::util::BuildUserLoginReq(req_param, hdr_param);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Buffer MsgBuilder::BuildUserLogoutReq(uint32_t seq)
{
	prot::util::UserLogoutReqParam req_param;
	req_param.app_id      = m_engine_param.app_id;
	req_param.client_id   = m_engine_param.client_id;
	req_param.register_id = m_engine_data.register_id;
	req_param.user_id     = m_engine_data.user_id;
	req_param.user_type   = 0; // TODO:
	req_param.login_id    = m_engine_data.login_id;
	req_param.token       = "unknown";

	com::SigHdrParam hdr_param;
	hdr_param.client_id = m_engine_param.client_id;
	hdr_param.app_id    = m_engine_param.app_id;
	hdr_param.user_id   = m_engine_data.user_id;
	hdr_param.seq       = seq;

	return prot::util::BuildUserLogoutReq(req_param, hdr_param);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Buffer MsgBuilder::BuildJoinGroupReq(uint32_t group_id, uint32_t seq,
	const std::vector<CamDevice>& cameras, const std::vector<MicDevice>& microphones)
{
	prot::util::JoinGroupReqParam req_param;
	req_param.app_id    = m_engine_param.app_id;
	req_param.group_id  = group_id;
	req_param.user_id   = m_engine_data.user_id;
	req_param.user_type = 0; // TODO:
	req_param.login_id  = m_engine_data.login_id;
	req_param.token     = "unknown";

	for (const auto& cam : cameras) {
		prot::util::MediaEntry entry;
		entry.media_src_type = (uint32_t)MediaSrcType::CAMERA;
		entry.media_src_id = std::to_string(cam.cam_id);
		req_param.media_entries.push_back(entry);
	}

	for (const auto& mic : microphones) {
		prot::util::MediaEntry entry;
		entry.media_src_type = (uint32_t)MediaSrcType::MICROPHONE;
		entry.media_src_id = std::to_string(mic.mic_id);
		req_param.media_entries.push_back(entry);
	}

	com::SigHdrParam hdr_param;
	hdr_param.app_id    = m_engine_param.app_id;
	hdr_param.group_id  = group_id;
	hdr_param.user_id   = m_engine_data.user_id;
	hdr_param.client_id = m_engine_param.client_id;
	hdr_param.seq       = seq;

	return prot::util::BuildJoinGroupReq(req_param, hdr_param);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer MsgBuilder::BuildLeaveGroupReq(uint32_t seq)
{
	prot::util::LeaveGroupReqParam req_param;
	req_param.app_id    = m_engine_param.app_id;
	req_param.group_id  = m_engine_data.group_id;
	req_param.user_id   = m_engine_data.user_id;
	req_param.user_type = 0; // TODO:
	req_param.login_id  = m_engine_data.login_id;
	req_param.token     = "unknown";

	com::SigHdrParam hdr_param;
	hdr_param.app_id    = m_engine_param.app_id;
	hdr_param.group_id  = m_engine_data.group_id;
	hdr_param.user_id   = m_engine_data.user_id;
	hdr_param.client_id = m_engine_param.client_id;
	hdr_param.seq       = seq;

	return prot::util::BuildLeaveGroupReq(req_param, hdr_param);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Buffer MsgBuilder::BuildClientRegReq(uint32_t seq)
{
	prot::util::ClientRegReqParam req_param;
	req_param.app_id      = m_engine_param.app_id;
	req_param.client_id   = m_engine_param.client_id;
	req_param.client_name = m_engine_param.client_name;
	req_param.client_type = 0; // TODO:
	req_param.secret      = "unknown";
	req_param.os          = "windows";
	req_param.version     = "1.0.0";
	req_param.device      = "PC";

	com::SigHdrParam hdr_param;
	hdr_param.app_id    = m_engine_param.app_id;
	hdr_param.client_id = m_engine_param.client_id;
	hdr_param.seq       = seq;

	return prot::util::BuildClientRegReq(req_param, hdr_param);
}

}