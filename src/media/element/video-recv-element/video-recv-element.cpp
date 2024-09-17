#include "video-recv-element.h"
#include "util-streamer.h"
#include "common/util-common.h"
#include "common/util-net.h"
#include "common/util-pb.h"
#include "net-message.h"
#include "protocol.h"
#include "common-message.h"
#include "transport-msg-builder.h"
#include "protoc/transport.pb.h"
#include "pipeline-msg.h"
#include "util-protocol.h"
#include "log.h"
#include "common/media-common-define.h"
#include "yaml-cpp/yaml.h"

using namespace jukey::com;
using namespace jukey::util;

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
VideoRecvElement::VideoRecvElement(base::IComFactory* factory, const char* owner)
	: ProxyUnknown(nullptr)
	, ComObjTracer(factory, CID_VIDEO_RECV, owner)
	, ElementBase(factory)
	, CommonThread("video recv element", false)
{
	m_ele_name   = CONSTRUCT_ELEMENT_NAME("v-recv-");
	m_main_type  = EleMainType::SRC;
	m_sub_type   = EleSubType::RECEIVER;
	m_media_type = EleMediaType::VIDEO_ONLY;

	if (ERR_CODE_OK != CreateSrcPin()) {
		m_pins_created = false;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
VideoRecvElement::~VideoRecvElement()
{
	LOG_INF("Destruct {}", m_ele_name);

	if (m_timer_id != INVALID_TIMER_ID) {
		m_timer_mgr->StopTimer(m_timer_id);
		m_timer_mgr->FreeTimer(m_timer_id);
		m_timer_id = INVALID_TIMER_ID;
	}

	if (m_session_id != INVALID_SESSION_ID) {
		m_sess_mgr->CloseSession(m_session_id);
	}

	StopThread();

	if (m_stream_receiver) {
		m_stream_receiver->Release();
		m_stream_receiver = nullptr;
	}

	m_async_proxy->Stop();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* VideoRecvElement::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_VIDEO_RECV) == 0) {
		return new VideoRecvElement(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* VideoRecvElement::NDQueryInterface(const char* riid)
{
	if (0 == strcmp(riid, IID_ELEMENT)) {
		return static_cast<IElement*>(this);
	}
	else {
		return ProxyUnknown::NDQueryInterface(riid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoRecvElement::CreateSrcPin()
{
	media::com::VideoCaps video_pin_caps;
	video_pin_caps.AddCap(media::VideoCodec::H264);
	video_pin_caps.AddCap(media::PixelFormat::I420);
	video_pin_caps.AddCap(media::VideoRes::RES_1920x1080);
	video_pin_caps.AddCap(media::VideoRes::RES_1280x720);
	video_pin_caps.AddCap(media::VideoRes::RES_640x480);
	video_pin_caps.AddCap(media::VideoRes::RES_640x360);
	video_pin_caps.AddCap(media::VideoRes::RES_320x240);
	video_pin_caps.AddCap(media::VideoRes::RES_320x180);

	ISrcPin* src_pin = (ISrcPin*)QI(CID_SRC_PIN, IID_SRC_PIN, m_ele_name);
	if (!src_pin) {
		LOG_ERR("Create src pin failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != src_pin->Init(media::MediaType::VIDEO,
			this,
			CONSTRUCT_PIN_NAME("v-src-pin-"),
			media::util::ToVideoCapsStr(video_pin_caps),
			this)) {
		LOG_ERR("Init src pin failed!");
		return ERR_CODE_FAILED;
	}

	m_src_pin_index = static_cast<uint32_t>(m_src_pins.size());
	m_src_pins.push_back(src_pin);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoRecvElement::ParseProperties(com::IProperty* props)
{
	m_sess_mgr = (net::ISessionMgr*)props->GetPtrValue("session-mgr");
	if (!m_sess_mgr) {
		LOG_ERR("Cannot find session-mgr property!");
		return ERR_CODE_FAILED;
	}

	const void* stream = props->GetPtrValue("media-stream");
	if (!stream) {
		LOG_ERR("Invalid net-stream property!");
		return ERR_CODE_FAILED;
	}
	m_stream_info = *((MediaStream*)stream);

	LOG_INF("Read property stream-info, app:{}, user:{}, src:{}|{}, stream:{}|{}",
		m_stream_info.src.app_id,
		m_stream_info.src.user_id,
		m_stream_info.src.src_type,
		m_stream_info.src.src_id,
		m_stream_info.stream.stream_type,
		m_stream_info.stream.stream_id);

	const char* vs = props->GetStrValue("service-addr");
	if (!vs) {
		LOG_ERR("Cannot find addr property!");
		return ERR_CODE_FAILED;
	}

	LOG_INF("Read property service-addr:{}", vs);

	std::optional<com::Address> addr = ParseAddress(vs);
	if (addr.has_value()) {
		m_service_addr = addr.value();
	}
	else {
		LOG_ERR("Parse service address failed!");
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::ParseElementConfig()
{
	try {
		YAML::Node root = YAML::LoadFile(ELEMENT_CONFIG_FILE);

		auto node = root["video-recv-element"];
		if (!node) {
			return;
		}

		if (node["dump-recv-data"]) {
			auto enable = node["dump-recv-data"].as<bool>();
			if (enable) {
				m_stream_dumper.reset(new media::util::StreamDumper(m_ele_name + ".data"));
			}
		}
	}
	catch (const std::exception& e) {
		LOG_WRN("Error:{}", e.what());
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoRecvElement::DoInit(com::IProperty* props)
{
	m_logger = g_logger;

	if (!props) {
		LOG_ERR("Invalid properties!");
		return ERR_CODE_INVALID_PARAM;
	}

	LOG_INF("DoInit, props:{}", props->Dump());

	if (ERR_CODE_OK != ParseProperties(props)) {
		LOG_ERR("Parse properties failed!");
		return ERR_CODE_FAILED;
	}

	m_timer_mgr = QUERY_TIMER_MGR(m_factory);
	assert(m_timer_mgr);

	com::TimerParam timer_param;
	timer_param.timeout = 1000;
	timer_param.timer_type = TimerType::TIMER_TYPE_LOOP;
	timer_param.timer_name = "video receive element timer";
	timer_param.timer_func = [this](int64_t) {
		this->NotifyStreamStats();
	};

	m_timer_id = m_timer_mgr->AllocTimer(timer_param);
	m_timer_mgr->StartTimer(m_timer_id);

	m_data_stats.reset(new DataStats(m_factory, g_logger, m_ele_name));
	m_data_stats->Start();

	StatsParam fr_stats("framerate", StatsType::IAVER, 5000);
	m_fr_stats = m_data_stats->AddStats(fr_stats);

	StatsParam br_stats("bitrate", StatsType::IAVER, 5000);
	m_br_stats = m_data_stats->AddStats(br_stats);

	StartThread();

	m_async_proxy.reset(new util::SessionAsyncProxy(m_factory, m_sess_mgr,
		this, 10000));

	m_pipeline->SubscribeMsg(PlMsgType::NEGOTIATE, this);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoRecvElement::DoStart()
{
	LOG_INF("DoStart");

	net::CreateParam param;
	param.remote_addr  = m_service_addr;
	param.ka_interval  = 5; // second
	param.service_type = ServiceType::TRANSPORT;
	param.session_type = net::SessionType::UNRELIABLE;
	param.thread       = this;

	m_session_id = m_sess_mgr->CreateSession(param);
	if (m_session_id == INVALID_SESSION_ID) {
		LOG_ERR("Create session failed!");
		return ERR_CODE_FAILED;
	}

	LOG_INF("Start to create session:{}", m_session_id);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::OnLogoutRecvChannelRsp(const com::Buffer& buf)
{
	prot::LoginRecvChannelRsp rsp;
	if (!rsp.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse register response failed!");
		return;
	}

	LOG_INF("Received login recv channel response, result:{}", rsp.result());

	m_recv_chnl_id = 0;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::OnLogoutRecvChannelTimeout()
{
	LOG_ERR("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::OnLogoutRecvChannelError(com::ErrCode ec)
{
	LOG_ERR("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoRecvElement::DoStop()
{
	LOG_INF("DoStop");

	m_timer_mgr->StopTimer(m_timer_id);
	m_timer_mgr->FreeTimer(m_timer_id);

	if (m_recv_chnl_id == 0) {
		LOG_INF("Not logined recv channel, do nothing");
		return ERR_CODE_OK;
	}

	prot::util::LogoutRecvChannelReqParam req_param;
	req_param.app_id     = m_stream_info.src.app_id;
	req_param.user_id    = m_stream_info.src.user_id;
	req_param.user_type  = 0; // TODO:
	req_param.channel_id = m_recv_chnl_id;
	req_param.stream     = m_stream_info;
	req_param.token      = "unknown";

	com::SigHdrParam hdr_param;
	hdr_param.app_id  = m_stream_info.src.app_id;
	hdr_param.user_id = m_stream_info.src.user_id;
	hdr_param.seq     = ++m_cur_seq;

	// Login send channel
	Buffer buf = prot::util::BuildLogoutRecvChannelReq(req_param, hdr_param);

	m_async_proxy->SendSessionMsg(m_session_id, buf, m_cur_seq,
		prot::MSG_LOGOUT_RECV_CHANNEL_RSP)
		.OnResponse([this](net::SessionId sid, const com::Buffer& rsp) {
			OnLogoutRecvChannelRsp(rsp);
		})
		.OnTimeout([this]() {
			OnLogoutRecvChannelTimeout();
		})
		.OnError([this](const std::string& err) {
			OnLogoutRecvChannelError(ERR_CODE_FAILED);
		});

	LOG_INF("Send logout recv channel request, seq:{}, app:{}, user:{}, "
		"media:{}|{}, stream:{}|{}",
		m_cur_seq, 
		m_stream_info.src.app_id,
		m_stream_info.src.user_id,
		m_stream_info.src.src_type,
		m_stream_info.src.src_id,
		m_stream_info.stream.stream_type,
		m_stream_info.stream.stream_id);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::OnVideoStreamData(const Buffer& buf)
{
	LOG_DBG("Recv video stream data:{}", buf.data_len);

	m_data_stats->OnData(m_br_stats, buf.data_len);

	// 剥掉 SigMsgHdr
	uint32_t buf_len = buf.data_len - sizeof(prot::SigMsgHdr);
	com::Buffer fec_buf(buf_len, buf_len);
	memcpy(DP(fec_buf), DP(buf) + sizeof(prot::SigMsgHdr), buf_len);

	m_stream_receiver->InputStreamData(fec_buf);

	if (!m_receiving_data) {
		m_receiving_data = true;
		NotifyRunState("Start receiving video data");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::OnVideoStreamFeedback(const Buffer& buf)
{
	// 剥掉 SigMsgHdr
	uint32_t buf_len = buf.data_len - SIG_HDR_LEN;
	com::Buffer fb_buf(buf_len, buf_len);
	memcpy(DP(fb_buf), DP(buf) + SIG_HDR_LEN, buf_len);

	m_stream_receiver->InputFeedback(fb_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::OnPauseRecvStreamRsp(const Buffer& buf)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::OnResumeRecvStreamRsp(const Buffer& buf)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::OnSessionData(const CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(net::SessionDataMsg);

	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(data->buf);

	if (sig_hdr->mt != prot::MSG_STREAM_DATA) {
		LOG_INF("Received session message, msg:{}, len:{}",
			prot::util::MSG_TYPE_STR(sig_hdr->mt), data->buf.data_len);
		prot::util::DumpSignalHeader(g_logger, sig_hdr);
	}

	switch (sig_hdr->mt) {
	case prot::MSG_STREAM_DATA:
		OnVideoStreamData(data->buf);
		break;
	case prot::MSG_STREAM_FEEDBACK:
		OnVideoStreamFeedback(data->buf);
		break;
	default:
		if (!m_async_proxy->OnSessionMsg(data->lsid, data->buf)) {
			LOG_ERR("Unknown protocol type:{}", (uint32_t)sig_hdr->mt);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::OnSessionClosed(const CommonMsg& msg)
{
	NotifyRunState("Video receive session closed!");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::OnVideoNegotiateRsp(const com::Buffer& buf)
{
	prot::NegotiateRsp rsp;
	if (!rsp.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse negotiate response failed!");
		return;
	}

	LOG_INF("Received negotiate response:{}", util::PbMsgToJson(rsp));

	if (rsp.result() != 0) {
		LOG_ERR("Video negotiate failed");
		return;
	}

	m_negotiated = true;

	// Notify stream

	std::string stream_id = m_stream_info.stream.stream_id;

	if (ERR_CODE_OK != SRC_PIN->OnPinMsg(nullptr,
		PinMsg(PinMsgType::SET_STREAM, stream_id))) {
		LOG_ERR("Set src pin:{} stream failed!", SRC_PIN->Name());
		return;
	}

	ElementStream* es = new ElementStream();
	es->stream.src = m_stream_info.src;
	es->stream.stream.stream_type = StreamType::VIDEO;
	es->stream.stream.stream_id = stream_id;
	es->pin.ele_name = m_ele_name;
	es->pin.pin_name = m_src_pins[0]->Name();
	es->cap = m_src_pins[0]->Cap();

	com::CommonMsg msg;
	msg.msg_type = (uint32_t)PlMsgType::ADD_ELEMENT_STREAM;
	msg.src = m_ele_name;
	msg.msg_data.reset(es);
	m_pipeline->PostPlMsg(msg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::OnVideoNegotiateTimeout()
{
	LOG_ERR("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::OnVideoNegotiateError(com::ErrCode ec)
{
	LOG_ERR("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::TryNegotiateWithServer()
{
	if (m_recv_chnl_id == 0) {
		LOG_INF("Not logined");
		return;
	}

	if (m_src_pins.empty()) {
		LOG_ERR("No src pin");
		return;
	}

	if (m_src_pins.front()->PrepCaps().empty()) {
		LOG_ERR("No prepared caps");
		return;
	}

	prot::util::NegotiateReqParam req_param;
	req_param.channel_id = m_recv_chnl_id;
	req_param.stream = m_stream_info;

	for (const auto& cap : m_src_pins.front()->PrepCaps()) {
		req_param.caps.push_back(cap);
	}

	com::SigHdrParam hdr_param;
	hdr_param.app_id = m_stream_info.src.app_id;
	hdr_param.user_id = m_stream_info.src.user_id;
	hdr_param.seq = ++m_cur_seq;

	Buffer buf = prot::util::BuildNegotiateReq(req_param, hdr_param);

	m_async_proxy->SendSessionMsg(m_session_id, buf, m_cur_seq,
		prot::MSG_NEGOTIATE_RSP)
		.OnResponse([this](net::SessionId sid, const com::Buffer& rsp) {
			OnVideoNegotiateRsp(rsp);
		})
		.OnTimeout([this]() {
			OnVideoNegotiateTimeout();
		})
		.OnError([this](const std::string& err) {
			OnVideoNegotiateError(ERR_CODE_FAILED);
		});

	LOG_INF("Send negotiate request, seq:{}, app:{}, user:{}, stream:{}|{}",
		m_cur_seq,
		m_stream_info.src.app_id,
		m_stream_info.src.user_id,
		m_stream_info.stream.stream_type,
		m_stream_info.stream.stream_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::OnLoginRecvChannelRsp(const Buffer& buf)
{
	jukey::prot::LoginRecvChannelRsp rsp;
	if (!rsp.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse login recv channel response failed!");
		return;
	}

	LOG_INF("Received login recv channel response, result:{}", rsp.result());

	if (rsp.result() != 0) {
		LOG_ERR("Login recv channel failed");
		return; // TODO:
	}

	m_stream_receiver = (txp::IStreamReceiver*)m_factory->CreateComponent(
		CID_STREAM_RECEIVER, m_ele_name);
	if (!m_stream_receiver) {
		LOG_ERR("Create stream receiver failed!");
		return;
	}

	if (ERR_CODE_OK != m_stream_receiver->Init(this, rsp.channel_id(), 
		rsp.user_id(), m_stream_info)) {
		LOG_ERR("Initialize stream sender failed!");
		return;
	}

	m_recv_chnl_id = rsp.channel_id();

	TryNegotiateWithServer();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::OnLoginRecvChannelTimeout()
{
	LOG_ERR("Login recv channel timeout");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::OnLoginRecvChannelError(ErrCode ec)
{
	LOG_ERR("Login recv channel error");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::OnSessionCreateResult(const CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(net::SessionCreateResultMsg);

	LOG_INF("Session create notify, lsid:{}, rsid:{}, result:{}", data->lsid,
		data->rsid, data->result);

	if (!data->result) {
		LOG_ERR("Create session failed!");
		NotifyRunState("Video receive session create failed!");
		return;
	}
	NotifyRunState("Video receive session create success");

	prot::util::LoginRecvChannelReqParam req_param;
	req_param.app_id = m_stream_info.src.app_id;
	req_param.user_id = m_stream_info.src.user_id;
	req_param.user_type = 0; // TODO:
	req_param.stream = m_stream_info;
	req_param.token = "unknown";

	com::SigHdrParam hdr_param;
	hdr_param.app_id = m_stream_info.src.app_id;
	hdr_param.user_id = m_stream_info.src.user_id;
	hdr_param.seq = ++m_cur_seq;

	// Login send channel
	Buffer buf = prot::util::BuildLoginRecvChannelReq(req_param, hdr_param);

	m_async_proxy->SendSessionMsg(data->lsid, buf, m_cur_seq,
		prot::MSG_LOGIN_RECV_CHANNEL_RSP)
		.OnResponse([this](net::SessionId sid, const com::Buffer& rsp) {
			OnLoginRecvChannelRsp(rsp);
		})
		.OnTimeout([this]() {
			OnLoginRecvChannelTimeout();
		})
		.OnError([this](const std::string& err) {
			OnLoginRecvChannelError(ERR_CODE_FAILED);
		});

	LOG_INF("Send login recv channel request, seq:{}, app:{}, user:{}, "
		"media:{}|{}, stream:{}|{}",
		m_cur_seq, 
		m_stream_info.src.app_id,
		m_stream_info.src.user_id,
		m_stream_info.src.src_type,
		m_stream_info.src.src_id,
		m_stream_info.stream.stream_type,
		m_stream_info.stream.stream_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::OnThreadMsg(const CommonMsg& msg)
{
	switch (msg.msg_type) {
	case net::NET_MSG_SESSION_CLOSED:
		OnSessionClosed(msg);
		break;
	case net::NET_MSG_SESSION_CREATE_RESULT:
		OnSessionCreateResult(msg);
		break;
	case net::NET_MSG_SESSION_DATA:
		OnSessionData(msg);
		break;
	case net::NET_MSG_SESSION_INCOMING:
		break; // noting to do
	default:
		LOG_ERR("Unknown message type:{}", msg.msg_type);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::OnStreamFrame(uint32_t channel_id, uint32_t user_id,
	const com::MediaStream& stream, const com::Buffer& buf)
{
	prot::VideoFrameHdr* hdr = (prot::VideoFrameHdr*)DP(buf);

	LOG_DBG("Received video frame, seq:{}, size:{}", hdr->fseq, buf.data_len);

	m_data_stats->OnData(m_fr_stats, 1);

	PinData pin_data(media::MediaType::VIDEO);

	pin_data.dts = hdr->ts;
	pin_data.pts = hdr->ts;
	pin_data.pos = 0;
	pin_data.drt = 0;
	pin_data.pos = 0;
	pin_data.syn = 0;
	pin_data.tbn = 0;
	pin_data.tbd = 0;

	pin_data.data_count = 1;

	com::Buffer& video_buf = const_cast<com::Buffer&>(buf);
	video_buf.start_pos += sizeof(prot::VideoFrameHdr);
	video_buf.data_len -= sizeof(prot::VideoFrameHdr);

	pin_data.media_data[0] = video_buf;

	m_last_frame_width = hdr->w;
	m_last_frame_height = hdr->h;

	if (m_ele_state == EleState::RUNNING) {
		SRC_PIN->OnPinData(pin_data);

		if (m_stream_dumper) {
			m_stream_dumper->WriteStreamData(pin_data);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::OnReceiverFeedback(uint32_t channel_id,
	uint32_t user_id, const com::MediaStream& stream, const com::Buffer& buf)
{
	LOG_INF("{}", __FUNCTION__);

	uint32_t hdr_len = sizeof(prot::SigMsgHdr);
	uint32_t buf_len = buf.data_len + hdr_len;

	com::Buffer sig_buf(buf_len, buf_len);

	// FIXME: 其他参数没有设置
	prot::SigMsgHdr* prot_hdr = (prot::SigMsgHdr*)(DP(sig_buf));
	prot_hdr->len = (uint16_t)(buf.data_len);
	prot_hdr->mt = prot::MSG_STREAM_FEEDBACK;
	prot_hdr->seq = ++m_cur_seq;

	memcpy(DP(sig_buf) + hdr_len, DP(buf), buf.data_len);

	m_sess_mgr->SendData(m_session_id, sig_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoRecvElement::OnPipelineNegotiateMsg(const com::CommonMsg& msg)
{
	if (ERR_CODE_OK != SRC_PIN->Negotiate()) {
		LOG_ERR("Update video caps failed!");
		msg.result->set_value(ERR_CODE_FAILED);
	}
	else {
		LOG_INF("Prepare negotiate success!");
		msg.result->set_value(ERR_CODE_OK);
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoRecvElement::PreProcPipelineMsg(const CommonMsg& msg)
{
	switch (msg.msg_type) {
	case PlMsgType::NEGOTIATE:
		return OnPipelineNegotiateMsg(msg);
	default:
		return ERR_CODE_MSG_NO_PROC;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoRecvElement::OnSrcPinNegotiated(ISrcPin* pin, const std::string& cap)
{
	LOG_INF("Src pin negotiated, cap:{}", media::util::Capper(cap));

	auto src_cap = media::util::ParseVideoCap(cap);
	if (!src_cap.has_value()) {
		LOG_ERR("Parse video cap failed");
		return ERR_CODE_FAILED;
	}

	m_src_pin_cap = src_cap.value();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRecvElement::NotifyStreamStats()
{
	if (m_ele_state != EleState::RUNNING) return;

	VideoStreamStatsData* data = new VideoStreamStatsData();
	data->stream = m_stream_info;
	data->stats.width = m_last_frame_width;
	data->stats.height = m_last_frame_height;

	com::CommonMsg msg;
	msg.msg_type = (uint32_t)PlMsgType::VIDEO_STREAM_STATS;
	msg.msg_data.reset(data);

	m_pipeline->PostPlMsg(msg);
}

}