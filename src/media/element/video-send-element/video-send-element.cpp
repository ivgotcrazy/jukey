#include "video-send-element.h"
#include "util-streamer.h"
#include "common/util-pb.h"
#include "common/util-net.h"
#include "net-message.h"
#include "protocol.h"
#include "transport-msg-builder.h"
#include "common-message.h"
#include "protoc/transport.pb.h"
#include "util-protocol.h"
#include "log.h"
#include "common/media-common-define.h"

using namespace jukey::com;

namespace jukey::stmr {

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
VideoSendElement::VideoSendElement(base::IComFactory* factory, const char* owner)
	: base::ProxyUnknown(nullptr)
	, base::ComObjTracer(factory, CID_VIDEO_SEND, owner)
	, CommonThread("video send element", false)
	, ElementBase(factory)
{
	m_ele_name   = CONSTRUCT_ELEMENT_NAME("v-send-");
	m_main_type  = EleMainType::SINK;
	m_sub_type   = EleSubType::SENDER;
	m_media_type = EleMediaType::VIDEO_ONLY;

	if (ERR_CODE_OK != CreateSinkPin()) {
		m_pins_created = false;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
VideoSendElement::~VideoSendElement()
{
	LOG_INF("Destruct {}", m_ele_name);

	StopThread();

	if (m_stream_sender) {
		m_stream_sender->Release();
		m_stream_sender = nullptr;
	}

	if (m_session_id != INVALID_SESSION_ID) {
		m_sess_mgr->CloseSession(m_session_id);
	}

	m_data_stats->Stop();
	m_async_proxy->Stop();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* VideoSendElement::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_VIDEO_SEND) == 0) {
		return new VideoSendElement(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* VideoSendElement::NDQueryInterface(const char* riid)
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
ErrCode VideoSendElement::CreateSinkPin()
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

	ISinkPin* sink_pin = (ISinkPin*)QI(CID_SINK_PIN, IID_SINK_PIN, m_ele_name);
	if (!sink_pin) {
		LOG_ERR("Create sink pin failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != sink_pin->Init(media::MediaType::VIDEO,
			this,
			CONSTRUCT_PIN_NAME("v-sink-pin-"),
			media::util::ToVideoCapsStr(video_pin_caps),
			this)) {
		LOG_ERR("Init sink pin failed!");
		return ERR_CODE_FAILED;
	}

	m_sink_pin_index = static_cast<uint32_t>(m_sink_pins.size());
	m_sink_pins.push_back(sink_pin);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoSendElement::ParseProperties(com::IProperty* props)
{
	m_sess_mgr = (net::ISessionMgr*)props->GetPtrValue("session-mgr");
	if (!m_sess_mgr) {
		LOG_ERR("Cannot find session-mgr property!");
		return ERR_CODE_FAILED;
	}

	const void* vp = props->GetPtrValue("media-stream");
	if (!vp) {
		LOG_ERR("Invalid stream-id property!");
		return ERR_CODE_OK;
	}
	m_stream_info = *((MediaStream*)vp);

	LOG_INF("Read property stream-info, app:{}, user:{}, stream:{}|{}",
		m_stream_info.src.app_id,
		m_stream_info.src.user_id,
		m_stream_info.stream.stream_type,
		m_stream_info.stream.stream_id);

	const char* vs = props->GetStrValue("service-addr");
	if (!vs) {
		LOG_ERR("Cannot find addr property!");
		return ERR_CODE_FAILED;
	}

	LOG_INF("Read property service-addr:{}", vs);

	std::optional<com::Address> addr = util::ParseAddress(vs);
	if (addr.has_value()) {
		m_service_addr = addr.value();
	}
	else {
		LOG_ERR("Parse service address failed!");
		return ERR_CODE_FAILED;
	}

	m_br_alloc_mgr = (IBitrateAllocateMgr*)props->GetPtrValue(
		"bitrate-allocate-mgr");
	if (!m_br_alloc_mgr) {
		LOG_INF("No birate allocate manager");
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoSendElement::DoInit(com::IProperty* props)
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

	m_data_stats.reset(new util::DataStats(m_factory, g_logger, m_ele_name));
	m_data_stats->Start();

	util::StatsParam fr_stats("frame-rate", util::StatsType::IAVER, 5000);
	m_fr_stats_id = m_data_stats->AddStats(fr_stats);
	util::StatsParam br_stats("bitrate", util::StatsType::IAVER, 5000);
	m_br_stats_id = m_data_stats->AddStats(br_stats);

	StartThread();

	m_async_proxy = std::make_shared<util::SessionAsyncProxy>(m_factory,
		m_sess_mgr, this, 10000);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoSendElement::DoStart()
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
void VideoSendElement::OnLogoutSendChannelRsp(const com::Buffer& buf)
{
	prot::LogoutSendChannelRsp rsp;
	if (!rsp.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse logout send channel response failed!");
		return;
	}

	LOG_INF("Received logout send channel response:{}", util::PbMsgToJson(rsp));

	m_send_chnl_id = 0;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoSendElement::OnLogoutSendChannelTimeout()
{
	LOG_ERR("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoSendElement::OnLogoutSendChannelError(com::ErrCode ec)
{
	LOG_ERR("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoSendElement::DoStop()
{
	LOG_INF("DoStop");

	if (m_send_chnl_id == 0) {
		LOG_INF("Not logined send channel, do nothing");
		return ERR_CODE_OK;
	}

	prot::util::LogoutSendChannelReqParam req_param;
	req_param.app_id    = m_stream_info.src.app_id;
	req_param.user_id   = m_stream_info.src.user_id;
	req_param.user_type = 0; // TODO:
	req_param.channe_id = m_send_chnl_id;
	req_param.stream    = m_stream_info;
	req_param.token     = "unknown"; // TODO:

	com::SigHdrParam hdr_param;
	hdr_param.app_id  = m_stream_info.src.app_id;
	hdr_param.user_id = m_stream_info.src.user_id;
	hdr_param.seq     = ++m_cur_seq;

	Buffer buf = prot::util::BuildLogoutSendChannelReq(req_param, hdr_param);

	m_async_proxy->SendSessionMsg(m_session_id, buf, m_cur_seq,
		prot::MSG_LOGOUT_SEND_CHANNEL_RSP)
		.OnResponse([this](net::SessionId sid, const com::Buffer& rsp) {
			OnLogoutSendChannelRsp(rsp);
		})
		.OnTimeout([this]() {
			OnLogoutSendChannelTimeout();
		})
		.OnError([this](const std::string& err) {
			OnLogoutSendChannelError(ERR_CODE_FAILED);
		});

	LOG_INF("Send logout send channel request, seq:{}, app:{}, user:{}, "
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
// TODO: 内存优化
//------------------------------------------------------------------------------
com::Buffer VideoSendElement::BuildVideoFrameData(media::VideoFrameParaSP para, 
	const com::Buffer& buf)
{
	uint32_t hdr_len = sizeof(prot::VideoFrameHdr);
	uint32_t buf_len = buf.data_len + hdr_len;

	com::Buffer new_buf(buf_len, buf_len);

	// Build frame header
	prot::VideoFrameHdr* frame_hdr = (prot::VideoFrameHdr*)(DP(new_buf));
	frame_hdr->ver   = 0;
	frame_hdr->ext   = 0;
	frame_hdr->ft    = para->key ? 1 : 0;
	frame_hdr->sl    = 0;
	frame_hdr->tl    = 0;
	frame_hdr->codec = (uint16_t)para->codec;
	frame_hdr->w     = para->width / 8;
	frame_hdr->h     = para->height / 8;
	frame_hdr->ts    = para->ts;
	frame_hdr->fseq  = para->seq;

	memcpy(DP(new_buf) + hdr_len, DP(buf), buf.data_len);

	return new_buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer VideoSendElement::BuildStreamDataMsg(const com::Buffer& buf)
{
	uint32_t hdr_len = sizeof(prot::SigMsgHdr);
	uint32_t buf_len = buf.data_len + hdr_len;

	com::Buffer new_buf(buf_len, buf_len);

	// FIXME: 其他参数没有设置
	prot::SigMsgHdr* prot_hdr = (prot::SigMsgHdr*)(DP(new_buf));
	prot_hdr->len = (uint16_t)(buf.data_len);
	prot_hdr->mt  = prot::MSG_STREAM_DATA;
	prot_hdr->seq = ++m_cur_seq;

	memcpy(DP(new_buf) + hdr_len, DP(buf), buf.data_len);

	return new_buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer VideoSendElement::BuildFeedbackMsg(const com::Buffer& buf)
{
	uint32_t hdr_len = sizeof(prot::SigMsgHdr);
	uint32_t buf_len = buf.data_len + hdr_len;

	com::Buffer new_buf(buf_len, buf_len);

	// FIXME: 其他参数没有设置
	prot::SigMsgHdr* prot_hdr = (prot::SigMsgHdr*)(DP(new_buf));
	prot_hdr->len = (uint16_t)(buf.data_len);
	prot_hdr->mt = prot::MSG_STREAM_FEEDBACK;
	prot_hdr->seq = ++m_cur_seq;

	memcpy(DP(new_buf) + hdr_len, DP(buf), buf.data_len);

	return new_buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoSendElement::OnSinkPinData(ISinkPin* pin, const PinData& data)
{
	if (m_ele_state != EleState::RUNNING) {
		LOG_ERR("Element is not running!");
		return ERR_CODE_FAILED;
	}

	m_data_stats->OnData(m_fr_stats_id, 1);

	if (m_stream_sender) {
		auto para = SPC<media::VideoFramePara>(data.media_para);
		com::Buffer frame_buf = BuildVideoFrameData(para, data.media_data[0]);
		m_stream_sender->InputFrameData(frame_buf);

		LOG_DBG("Send video frame, seq:{}, size:{}", para->seq, frame_buf.data_len);
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoSendElement::OnVideoNegotiateRsp(const com::Buffer& buf)
{
	prot::NegotiateRsp rsp;
	if (!rsp.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse negotiate response failed!");
		return;
	}

	LOG_INF("Received negotiate response:{}", util::PbMsgToJson(rsp));

	if (rsp.result() != 0) {
		LOG_ERR("Negotiate failed");
		return;
	}

	m_negotiated = true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoSendElement::OnVideoNegotiateTimeout()
{
	LOG_ERR("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoSendElement::OnVideoNegotiateError(com::ErrCode ec)
{
	LOG_ERR("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoSendElement::TryNegotiateWithServer()
{
	if (m_send_chnl_id == 0) {
		LOG_INF("Not logined");
		return;
	}

	if (!m_sink_pins.front()->Negotiated()) {
		LOG_INF("Not negotiated");
		return;
	}

	prot::util::NegotiateReqParam req_param;
	req_param.channel_id = m_send_chnl_id;
	req_param.stream = m_stream_info;
	req_param.caps.push_back(media::util::ToVideoCapStr(m_sink_pin_cap));
	
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

	LOG_INF("Send video negotiate request, seq:{}, app:{}, user:{}, media:{}|{}, "
		"stream:{}|{}",
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
ErrCode 
VideoSendElement::OnSinkPinNegotiated(ISinkPin* pin, const std::string& cap)
{
	LOG_INF("Sink pin negotiated, cap:{}", media::util::Capper(cap));

	auto sink_cap = media::util::ParseVideoCap(cap);
	if (!sink_cap.has_value()) {
		LOG_ERR("Parse video cap failed!");
		return ERR_CODE_FAILED;
	}

	m_sink_pin_cap = sink_cap.value();

	TryNegotiateWithServer();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoSendElement::OnVideoStreamFeedback(const Buffer& buf)
{
	uint32_t buf_len = buf.data_len - sizeof(prot::SigMsgHdr);

	com::Buffer fb_buf(buf_len, buf_len);

	memcpy(DP(fb_buf), DP(buf) + sizeof(prot::SigMsgHdr), buf_len);

	m_stream_sender->InputFeedbackData(fb_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoSendElement::OnStartSendStreamNotify(const Buffer& buf)
{
	prot::StartSendStreamNotify notify;
	if (!notify.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse start send stream notify failed!");
		return;
	}

	LOG_INF("Received start send stream notify:{}", util::PbMsgToJson(notify));

	// Start send stream
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoSendElement::OnStopSendStreamNotify(const Buffer& buf)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoSendElement::OnSessionData(const CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(net::SessionDataMsg);

	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(data->buf);

	LOG_INF("Received session message, msg:{}, len:{}",
		prot::util::MSG_TYPE_STR(sig_hdr->mt), data->buf.data_len);

	prot::util::DumpSignalHeader(g_logger, sig_hdr);

	switch (sig_hdr->mt) {
	case prot::MSG_STREAM_FEEDBACK:
		OnVideoStreamFeedback(data->buf);
		break;
	case prot::MSG_START_SEND_STREAM_NOTIFY:
		OnStartSendStreamNotify(data->buf);
		break;
	case prot::MSG_STOP_SEND_STREAM_NOTIFY:
		OnStopSendStreamNotify(data->buf);
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
void VideoSendElement::OnSessionClosed(const CommonMsg& msg)
{
	NotifyRunState("Video send session closed!");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoSendElement::OnLoginSendChannelRsp(const Buffer& buf)
{
	prot::LoginSendChannelRsp rsp;
	if (!rsp.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse login send channel response failed!");
		return;
	}

	LOG_INF("Received login send channel response:{}", util::PbMsgToJson(rsp));

	m_stream_sender = (txp::IStreamSender*)QI(CID_STREAM_SENDER,
		IID_STREAM_SENDER, m_ele_name);
	if (!m_stream_sender) {
		LOG_ERR("Create stream sender failed!");
		return;
	}

	if (ERR_CODE_OK != m_stream_sender->Init(this, rsp.channel_id(),
		rsp.user_id(), m_stream_info)) {
		LOG_ERR("Initialize stream sender failed!");
		return;
	}

	m_send_chnl_id = rsp.channel_id();

	TryNegotiateWithServer();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoSendElement::OnLoginSendChannelTimeout()
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoSendElement::OnLoginSendChannelError(ErrCode ec)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoSendElement::OnSessionCreateResult(const CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(net::SessionCreateResultMsg);

	if (!data->result) {
		// TODO:
		LOG_ERR("Session create failed, lsid:{}, rsid:{}", data->lsid, data->rsid);
		NotifyRunState("Video send session create failed!");
		return;
	}
	NotifyRunState("Video send session create success");

	LOG_INF("Session create success, lsid:{}, rsid:{}", data->lsid, data->rsid);

	prot::util::LoginSendChannelReqParam req_param;
	req_param.app_id = m_stream_info.src.app_id;
	req_param.user_id = m_stream_info.src.user_id;
	req_param.user_type = 0; // TODO:
	req_param.stream = m_stream_info;
	req_param.token = "unknown"; // TODO:

	com::SigHdrParam hdr_param;
	hdr_param.app_id = m_stream_info.src.app_id;
	hdr_param.user_id = m_stream_info.src.user_id;
	hdr_param.seq = ++m_cur_seq;

	// Login send channel
	Buffer buf = prot::util::BuildLoginSendChannelReq(req_param, hdr_param);

	m_async_proxy->SendSessionMsg(data->lsid, buf, m_cur_seq,
		prot::MSG_LOGIN_SEND_CHANNEL_RSP)
		.OnResponse([this](net::SessionId sid, const com::Buffer& rsp) {
			OnLoginSendChannelRsp(rsp);
		})
		.OnTimeout([this]() {
			OnLoginSendChannelTimeout();
		})
		.OnError([this](const std::string& err) {
			OnLoginSendChannelError(ERR_CODE_FAILED);
		});

	LOG_INF("Send login send channel request, seq:{}, app:{}, user:{}, "
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
void VideoSendElement::OnThreadMsg(const CommonMsg& msg)
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
		break;
	default:
		LOG_ERR("Unknown message type:{}", msg.msg_type);
	}
}

//------------------------------------------------------------------------------
// IStreamSenderHandler
//------------------------------------------------------------------------------
void VideoSendElement::OnStreamData(uint32_t channel_id, uint32_t user_id,
	const MediaStream& stream, const Buffer& buf)
{
	if (!m_negotiated) return;
	
	m_sess_mgr->SendData(m_session_id, BuildStreamDataMsg(buf));
	if (!m_sending_data) {
		m_sending_data = true;
		NotifyRunState("Start sending video data");
	}

	m_data_stats->OnData(m_br_stats_id, buf.data_len);
}

//------------------------------------------------------------------------------
// IStreamSenderHandler
//------------------------------------------------------------------------------
void VideoSendElement::OnSenderFeedback(uint32_t channel_id, uint32_t user_id,
	const MediaStream& stream, const Buffer& buf)
{
	m_sess_mgr->SendData(m_session_id, BuildFeedbackMsg(buf));
}

//------------------------------------------------------------------------------
// IStreamSenderHandler
//------------------------------------------------------------------------------
void VideoSendElement::OnEncoderTargetBitrate(uint32_t channel_id, 
	uint32_t user_id, const com::MediaStream& stream, uint32_t bw_kbps)
{
	LOG_INF("Encoder target bitrate update:{}", bw_kbps);	

	if (m_br_alloc_mgr) {
		m_br_alloc_mgr->UpdateBitrate(stream.stream.stream_id, bw_kbps);
	}
}

}