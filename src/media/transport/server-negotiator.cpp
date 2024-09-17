#include "server-negotiator.h"
#include "log.h"
#include "util-streamer.h"
#include "common/util-pb.h"
#include "common/util-time.h"
#include "stream-server.h"
#include "transport-msg-builder.h"


using namespace jukey::com;

#define GET(obj) (m_stream_server.obj)


namespace
{

jukey::stmr::PinCaps GetPinCaps(const jukey::prot::NegotiateReq& req)
{
	jukey::stmr::PinCaps pin_caps;

	for (auto it = req.caps().begin(); it != req.caps().end(); it++) {
		pin_caps.push_back(*it);
	}

	return pin_caps;
}

void TimerCallback(std::shared_ptr<void> owner, int64_t param)
{
	auto negotiator = SPC<jukey::txp::ServerNegotiator>(owner);
	negotiator->OnNegoTimer();
}

}

namespace jukey::txp
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ServerNegotiator::ServerNegotiator(StreamServer& server)
	: m_stream_server(server)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ServerNegotiator::~ServerNegotiator()
{
	LOG_INF("{}", __FUNCTION__);

	if (m_timer_id != INVALID_TIMER_ID) {
		GET(m_timer_mgr)->StopTimer(m_timer_id);
		GET(m_timer_mgr)->FreeTimer(m_timer_id);
		m_timer_id = INVALID_TIMER_ID;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerNegotiator::SendNegoRsp(uint32_t channel_id, prot::SigMsgHdr* sig_hdr, 
	ErrCode result, const std::string& msg, const std::string& cap)
{
	prot::util::NegotiateRspParam param;
	param.channel_id = channel_id;
	param.stream = GET(m_stream);
	param.result = result;
	param.msg = msg;
	param.cap = cap;

	com::SigHdrParam hdr;
	hdr.app_id = sig_hdr->app;
	hdr.user_id = sig_hdr->usr;
	hdr.seq = sig_hdr->seq;

	Buffer buf = prot::util::BuildNegotiateRsp(param, hdr);

	GET(m_handler)->OnSendChannelMsg(channel_id, sig_hdr->usr, buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerNegotiator::RecverNegoReqOnInitState(prot::SigMsgHdr* sig_hdr,
	const prot::NegotiateReq& req)
{
	LOG_INF("[{}] Received receiver negotiate request on INIT state",
		STRM_ID(GET(m_stream)));

	GET(m_receiver).nego_cap.clear();
	GET(m_receiver).avai_caps.clear();

	LOG_INF("Update available caps:");
	GET(m_receiver).avai_caps = GetPinCaps(req);

	GET(m_receiver).nego_cap = GET(m_receiver).avai_caps.front();

	LOG_INF("Negotiated cap:{}", media::util::Capper(GET(m_receiver).nego_cap));

	GET(m_nego_state) = NegoState::NEGO_STATE_DONE;

	// Send receiver negotiate response
	SendNegoRsp(req.channel_id(), sig_hdr, ERR_CODE_OK, "success",
		GET(m_receiver).nego_cap);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string ServerNegotiator::DoNegotiate()
{
	std::string nego_cap;

	for (const auto& cap : GET(m_receiver).avai_caps) {
		bool found_cap = true;
		for (auto item : GET(m_senders)) {
			bool found_avai = false;
			for (auto avai_cap : item.second->avai_caps) {
				if (avai_cap == cap) {
					found_avai = true;
					break;
				}
			}
			if (!found_avai) {
				LOG_WRN("Sender does not have the cap, channel:{}, cap:{}", 
					item.second->stream_sender->ChannelId(), cap);
				found_cap = false;
				break;
			}
		}

		if (found_cap) {
			nego_cap = cap;
			LOG_INF("Found negotiated cap:{}", cap);
			break;
		}
	}

	return nego_cap;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerNegotiator::RecverNegoReqOnWaitState(prot::SigMsgHdr* sig_hdr,
	const prot::NegotiateReq& req)
{
	LOG_INF("[{}] Received receiver negotiate request on WAIT state",
		STRM_ID(GET(m_stream)));

	GET(m_receiver).nego_cap.clear();
	GET(m_receiver).avai_caps.clear();

	LOG_INF("Update available caps:");
	GET(m_receiver).avai_caps = GetPinCaps(req);

	std::string nego_cap = DoNegotiate();
	if (nego_cap.empty()) {
		LOG_ERR("Negotiate failed");

		// Send sender negotiate response
		SendNegoRsp(GET(m_receiver).stream_receiver->ChannelId(), sig_hdr,
			ERR_CODE_FAILED, "failed", "");

		// Send receiver negotiate response
		for (auto& item : GET(m_senders)) {
			item.second->avai_caps.clear();
			item.second->nego_cap.clear();

			SendNegoRsp(item.second->stream_sender->ChannelId(), 
				&(item.second->sig_hdr), ERR_CODE_FAILED, "failed", "");
		}

		GET(m_nego_state) = NegoState::NEGO_STATE_INIT;
	}
	else {
		LOG_INF("Negotiate success");

		// Send sender negotiate response
		SendNegoRsp(GET(m_receiver).stream_receiver->ChannelId(), sig_hdr,
			ERR_CODE_OK, "success", nego_cap);

		GET(m_receiver).nego_cap = nego_cap;

		// Send receiver negotiate response
		for (auto& item : GET(m_senders)) {
			item.second->avai_caps.clear();
			item.second->nego_cap.clear();

			SendNegoRsp(item.second->stream_sender->ChannelId(),
				&(item.second->sig_hdr), ERR_CODE_OK, "success", nego_cap);

			item.second->nego_cap = nego_cap;
		}

		GET(m_nego_state) = NegoState::NEGO_STATE_DONE;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerNegotiator::RecverNegoReqOnDoneState(prot::SigMsgHdr* sig_hdr,
	const prot::NegotiateReq& req)
{
	LOG_INF("[{}] Received receiver negotiate request on DONE state",
		STRM_ID(GET(m_stream)));

	if (GET(m_receiver).avai_caps.empty()) {
		LOG_ERR("Empty available caps");
		return;
	}

	if (GET(m_receiver).nego_cap.empty()) {
		LOG_ERR("Empty negotiated cap");
		return;
	}

	LOG_INF("Re-negotiate of receiver, Old negotiated cap:{}",
		media::util::Capper(GET(m_receiver).nego_cap));

	LOG_INF("Clear old available caps:");
	for (const auto& cap : GET(m_receiver).avai_caps) {
		LOG_INF("{}", media::util::Capper(cap));
	}
	GET(m_receiver).avai_caps.clear();

	LOG_INF("Add new available caps:");
	GET(m_receiver).avai_caps = GetPinCaps(req);

	bool found = false;
	for (const auto& cap : GET(m_receiver).avai_caps) {
		if (cap == GET(m_receiver).nego_cap) {
			found = true;
			break;
		}
	}

	if (!found) {
		LOG_INF("Cannot find old negotiated cap, start to make new negotiation");
		// TODO: make negotiation
	}
	else {
		LOG_INF("Keep the same negotiated cap");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerNegotiator::ProcRecverNegoReq(prot::SigMsgHdr* sig_hdr,
	const prot::NegotiateReq& req)
{
	switch (GET(m_nego_state)) {
	case NegoState::NEGO_STATE_INIT:
		RecverNegoReqOnInitState(sig_hdr, req);
		break;
	case NegoState::NEGO_STATE_WAIT:
		RecverNegoReqOnWaitState(sig_hdr, req);
		break;
	case NegoState::NEGO_STATE_DONE:
		RecverNegoReqOnDoneState(sig_hdr, req);
		break;
	default:
		LOG_ERR("Invalid negotiation state:{}", GET(m_nego_state));
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerNegotiator::OnNegoTimer()
{
	// FIXME: dangle pointer???
	GET(m_thread)->Execute([this](std::shared_ptr<void> owner) -> void {
		if (GET(m_nego_state) == NegoState::NEGO_STATE_WAIT) {
			for (auto& item : GET(m_senders)) {
				SendNegoRsp(item.second->stream_sender->ChannelId(),
					&(item.second->sig_hdr), ERR_CODE_FAILED, "timeout", "");
			}
			GET(m_nego_state) = NegoState::NEGO_STATE_INIT;

			LOG_ERR("Negotiate wait timeout");
		}
		else {
			LOG_INF("Negotiate timer come, negotiate state:{}", GET(m_nego_state));
		}

		// Don't do it on timer thread, it will cause dead lock
		GET(m_timer_mgr)->StopTimer(m_timer_id);
		GET(m_timer_mgr)->FreeTimer(m_timer_id);
		m_timer_id = INVALID_TIMER_ID;
	}, shared_from_this());
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerNegotiator::SenderNegoReqOnInitState(SenderInfoSP info,
	prot::SigMsgHdr* sig_hdr, const prot::NegotiateReq& req)
{
	LOG_INF("[{}] Received sender negotiate request on INIT state",
		STRM_ID(GET(m_stream)));

	if (!info->avai_caps.empty()) {
		LOG_WRN("Sender's available caps is not empty");
		info->avai_caps.clear();
	}

	if (!info->nego_cap.empty()) {
		LOG_WRN("Sender's negotiated cap is not empty");
		info->nego_cap.clear();
	}

	info->sig_hdr = *sig_hdr;
	info->avai_caps = GetPinCaps(req);
	
	GET(m_nego_state) = NegoState::NEGO_STATE_WAIT;

	TimerParam timer_param;
	timer_param.timeout    = 10000; // 10s
	timer_param.timer_type = com::TimerType::TIMER_TYPE_ONCE;
	timer_param.timer_name = "Negotiate wait timer";
	timer_param.run_atonce = false;
	timer_param.timer_func = [this](int64_t) {
		OnNegoTimer();
	};

	m_timer_id = GET(m_timer_mgr)->AllocTimer(timer_param);
	GET(m_timer_mgr)->StartTimer(m_timer_id);

	LOG_INF("Start negotiate wait timer");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerNegotiator::SenderNegoReqOnWaitState(SenderInfoSP info,
	prot::SigMsgHdr* sig_hdr, const prot::NegotiateReq& req)
{
	LOG_INF("[{}] Received sender negotiate request on WAIT state",
		STRM_ID(GET(m_stream)));

	if (!info->avai_caps.empty()) {
		LOG_WRN("Sender's available caps is not empty");
		info->avai_caps.clear();
	}

	if (!info->nego_cap.empty()) {
		LOG_WRN("Sender's negotiated cap is not empty");
		info->nego_cap.clear();
	}

	info->sig_hdr = *sig_hdr;
	info->avai_caps = GetPinCaps(req);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerNegotiator::SenderNegoReqOnDoneState(SenderInfoSP info,
	prot::SigMsgHdr* sig_hdr, const prot::NegotiateReq& req)
{
	LOG_INF("[{}] Received sender negotiate request on DONE state",
		STRM_ID(GET(m_stream)));

	if (GET(m_receiver).nego_cap.empty()) {
		LOG_ERR("Empty receiver negotiated cap");
		return;
	}

	// Update available caps
	info->avai_caps = GetPinCaps(req);

	bool found = false;
	for (const auto& cap : info->avai_caps) {
		if (cap == GET(m_receiver).nego_cap) {
			found = true;
			break;
		}
	}

	if (found) {
		LOG_INF("Available caps matched the negotiated cap");
		info->nego_cap = GET(m_receiver).nego_cap;
		SendNegoRsp(req.channel_id(), sig_hdr, ERR_CODE_OK, "success", info->nego_cap);
	}
	else {
		LOG_ERR("Available caps can't match the negotiated cap");
		SendNegoRsp(req.channel_id(), sig_hdr, ERR_CODE_FAILED, "failed", "");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerNegotiator::ProcSenderNegoReq(uint32_t channel_id,
	prot::SigMsgHdr* sig_hdr, const prot::NegotiateReq& req)
{
	auto iter = GET(m_senders).find(channel_id);
	if (iter == GET(m_senders).end()) {
		LOG_ERR("Cannot find sender channel:{}", channel_id);
		return;
	}

	switch (GET(m_nego_state)) {
	case NegoState::NEGO_STATE_INIT:
		SenderNegoReqOnInitState(iter->second, sig_hdr, req);
		break;
	case NegoState::NEGO_STATE_WAIT:
		SenderNegoReqOnWaitState(iter->second, sig_hdr, req);
		break;
	case NegoState::NEGO_STATE_DONE:
		SenderNegoReqOnDoneState(iter->second, sig_hdr, req);
		break;
	default:
		LOG_ERR("Invalid negotiation state:{}", GET(m_nego_state));
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerNegotiator::OnRecvNegoReq(uint32_t channel_id, const Buffer& buf)
{
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(buf);

	prot::NegotiateReq req;
	if (!req.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse video negotiate request failed!");
		return;
	}

	LOG_INF("Received video negotiate request:{}", util::PbMsgToJson(req));

	if (req.mutable_caps()->empty()) {
		LOG_ERR("Empty video caps");
		return; // TODO:
	}

	if (GET(m_receiver).stream_receiver && 
		channel_id == GET(m_receiver).stream_receiver->ChannelId()) {
		ProcRecverNegoReq(sig_hdr, req);
	}
	else {
		ProcSenderNegoReq(channel_id, sig_hdr, req);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerNegotiator::OnRecvNegoRsp(uint32_t channel_id, const Buffer& buf)
{

}

}