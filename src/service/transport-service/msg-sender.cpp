#include "msg-sender.h"
#include "common/util-net.h"
#include "common/util-pb.h"
#include "log.h"
#include "protocol.h"
#include "msg-builder.h"
#include "transport-msg-builder.h"

using namespace jukey::com;

namespace jukey::srv
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MsgSender::MsgSender(net::ISessionMgr* session_mgr)
	: m_sess_mgr(session_mgr)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendLoginRecvChnlRsp(net::SessionId sid,
	uint32_t channel_id,
	const Buffer& buf,
	const prot::LoginRecvChannelReq& req,
	com::ErrCode result,
	const std::string& msg)
{
	prot::util::LoginRecvChannelRspParam rsp_param;
	rsp_param.app_id = req.app_id();
	rsp_param.user_id = req.user_id();
	rsp_param.user_type = req.user_type();
	rsp_param.channel_id = channel_id;
	rsp_param.result = result;
	rsp_param.msg = msg;
	rsp_param.stream = util::ToMediaStream(req.stream());

	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(buf);

	com::SigHdrParam hdr_param;
	hdr_param.app_id = sig_hdr->app;
	hdr_param.user_id = sig_hdr->usr;
	hdr_param.seq = sig_hdr->seq;

	Buffer rsp = prot::util::BuildLoginRecvChannelRsp(rsp_param, hdr_param);

	if (ERR_CODE_OK != m_sess_mgr->SendData(sid, rsp)) {
		LOG_ERR("Send login recv channel response failed!");
	}

	LOG_INF("Send login recv channel response, channel:{}", channel_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendLoginSendChnlRsp(net::SessionId sid,
	uint32_t channel_id,
	const Buffer& buf,
	const prot::LoginSendChannelReq& req,
	com::ErrCode result,
	const std::string& msg)
{
	LOG_INF("Send login send channel response");

	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(buf);

	prot::util::LoginSendChannelRspParam rsp_param;
	rsp_param.app_id = req.app_id();
	rsp_param.user_id = req.user_id();
	rsp_param.user_type = req.user_type();
	rsp_param.channel_id = channel_id;
	rsp_param.stream = util::ToMediaStream(req.stream());
	rsp_param.result = result;
	rsp_param.msg = msg;

	com::SigHdrParam hdr_param;
	hdr_param.app_id = req.app_id();
	hdr_param.user_id = req.user_id();
	hdr_param.client_id = sig_hdr->clt;
	hdr_param.seq = sig_hdr->seq;

	Buffer rsp = prot::util::BuildLoginSendChannelRsp(rsp_param, hdr_param);

	if (ERR_CODE_OK != m_sess_mgr->SendData(sid, rsp)) {
		LOG_ERR("Send login send channel response failed!");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendLogoutSendChnlRsp(net::SessionId sid,
	uint32_t channel_id,
	const Buffer& buf,
	const prot::LogoutSendChannelReq& req,
	com::ErrCode result,
	const std::string& msg)
{
	LOG_INF("Send logout send channel response");

	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(buf);

	prot::util::LogoutSendChannelRspParam rsp_param;
	rsp_param.app_id = req.app_id();
	rsp_param.user_id = req.user_id();
	rsp_param.user_type = req.user_type();
	rsp_param.channel_id = channel_id;
	rsp_param.stream = util::ToMediaStream(req.stream());
	rsp_param.result = result;
	rsp_param.msg = msg;

	com::SigHdrParam hdr_param;
	hdr_param.app_id = req.app_id();
	hdr_param.user_id = req.user_id();
	hdr_param.client_id = sig_hdr->clt;
	hdr_param.seq = sig_hdr->seq;

	Buffer rsp = prot::util::BuildLogoutSendChannelRsp(rsp_param, hdr_param);

	if (ERR_CODE_OK != m_sess_mgr->SendData(sid, rsp)) {
		LOG_ERR("Send logout send channel response failed!");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MsgSender::SendStartSendStreamNotify(uint32_t seq, net::SessionId sid,
	uint32_t channel_id, const Buffer& buf, const prot::LoginSendChannelReq& req)
{
	LOG_INF("Send start send stream notify");

	prot::util::StartSendStreamNotifyParam notify_param;
	notify_param.channel_id = channel_id;
	notify_param.stream = util::ToMediaStream(req.stream());

	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(buf);

	com::SigHdrParam hdr_param;
	hdr_param.app_id = req.app_id();
	hdr_param.user_id = req.user_id();
	hdr_param.client_id = sig_hdr->clt;
	hdr_param.seq = seq;

	Buffer notify = prot::util::BuildStartSendStreamNotify(notify_param,
		hdr_param);

	if (ERR_CODE_OK != m_sess_mgr->SendData(sid, notify)) {
		LOG_ERR("Send start send strean notify failed!");
	}
}

}