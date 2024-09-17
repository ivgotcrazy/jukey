#include "audio-send-element.h"
#include "util-streamer.h"
#include "util-streamer.h"
#include "common/util-common.h"
#include "common/util-net.h"
#include "common/util-pb.h"
#include "net-message.h"
#include "protocol.h"
#include "transport-msg-builder.h"
#include "common-message.h"
#include "protoc/transport.pb.h"
#include "util-protocol.h"
#include "log.h"
#include "common/media-common-define.h"
#include "yaml-cpp/yaml.h"
#include "protocol.h"
#include "pipeline-msg.h"


using namespace jukey::util;
using namespace jukey::com;

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AudioSendElement::AudioSendElement(base::IComFactory* factory, const char* owner)
	: base::ProxyUnknown(nullptr)
	, base::ComObjTracer(factory, CID_AUDIO_SEND, owner)
	, CommonThread("audio send element", false)
	, ElementBase(factory)
{
	m_ele_name   = CONSTRUCT_ELEMENT_NAME("a-send-");
	m_main_type  = EleMainType::SINK;
	m_sub_type   = EleSubType::SENDER;
	m_media_type = EleMediaType::AUDIO_ONLY;

	if (ERR_CODE_OK != CreateSinkPin()) {
		m_pins_created = false;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AudioSendElement::~AudioSendElement()
{
	LOG_INF("Destruct {}", m_ele_name);

	if (m_stream_sender) {
		m_stream_sender->Release();
		m_stream_sender = nullptr;
	}

	m_async_proxy->Stop();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* AudioSendElement::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_AUDIO_SEND) == 0) {
		return new AudioSendElement(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* AudioSendElement::NDQueryInterface(const char* riid)
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
ErrCode AudioSendElement::CreateSinkPin()
{
	media::com::AudioCaps src_pin_caps;
	src_pin_caps.AddCap(media::AudioCodec::OPUS);
	src_pin_caps.AddCap(media::AudioChnls::STEREO);
	src_pin_caps.AddCap(media::AudioChnls::MONO);
	src_pin_caps.AddCap(media::AudioSBits::S16);
	src_pin_caps.AddCap(media::AudioSBits::S32);
	src_pin_caps.AddCap(media::AudioSBits::S16P);
	src_pin_caps.AddCap(media::AudioSBits::S32P);
	src_pin_caps.AddCap(media::AudioSBits::FLTP);
	src_pin_caps.AddCap(media::AudioSRate::SR_48K);
	src_pin_caps.AddCap(media::AudioSRate::SR_16K);

	ISinkPin* sink_pin = (ISinkPin*)QI(CID_SINK_PIN, IID_SINK_PIN, m_ele_name);
	if (!sink_pin) {
		LOG_ERR("Create sink pin failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != sink_pin->Init(media::MediaType::AUDIO,
			this,
			CONSTRUCT_PIN_NAME("a-sink-pin-"),
			media::util::ToAudioCapsStr(src_pin_caps),
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
ErrCode AudioSendElement::ParseProperties(com::IProperty* props)
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
	m_stream_info = *((com::MediaStream*)vp);

	LOG_INF("Read property net-stream, media:{}|{}, stream:{}|{}",
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

	std::optional<com::Address> addr = util::ParseAddress(vs);
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
void AudioSendElement::ParseElementConfig()
{
	try {
		YAML::Node root = YAML::LoadFile(ELEMENT_CONFIG_FILE);

		auto node = root["audio-send-element"];
		if (!node) {
			return;
		}

		if (node["dump-send-data"]) {
			if (node["dump-send-data"].as<bool>()) {
				m_send_dumper.reset(new util::DataDumper(m_ele_name + ".data"));
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
ErrCode AudioSendElement::DoInit(com::IProperty* props)
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

	ParseElementConfig();

	m_data_stats.reset(new DataStats(m_factory, g_logger, m_ele_name));
	m_data_stats->Start();

	StatsParam br_stats("bitrate", StatsType::IAVER, 5000);
	m_br_stats = m_data_stats->AddStats(br_stats);

	StatsParam fc_stats("frame-count", StatsType::IACCU, 5000);
	m_fc_stats = m_data_stats->AddStats(fc_stats);

	StatsParam bc_stats("bit-count", StatsType::IACCU, 5000);
	m_bc_stats = m_data_stats->AddStats(bc_stats);

	StatsParam sn_stats("sn", StatsType::ISNAP, 5000);
	m_sn_stats = m_data_stats->AddStats(sn_stats);

	StartThread();

	m_async_proxy.reset(new util::SessionAsyncProxy(m_factory, m_sess_mgr,
		this, 10000));

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioSendElement::DoStart()
{
	LOG_INF("DoStart");

	net::CreateParam param;
	param.remote_addr  = m_service_addr;
	param.ka_interval  = 5; // second
	param.service_type = ServiceType::TRANSPORT;
	param.session_type = net::SessionType::RELIABLE;
	param.thread       = this;

	m_session_id = m_sess_mgr->CreateSession(param);
	if (m_session_id == INVALID_SESSION_ID) {
		LOG_ERR("Create session failed!");
		return ERR_CODE_FAILED;
	}

	LOG_INF("Start create session");

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioSendElement::DoStop()
{
	LOG_INF("DoStop");

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// TODO: 内存优化
//------------------------------------------------------------------------------
com::Buffer AudioSendElement::BuildAudioFrameData(media::AudioFrameParaSP para,
	const com::Buffer& buf)
{
	uint32_t hdr_len = sizeof(prot::AudioFrameHdr);
	uint32_t buf_len = buf.data_len + hdr_len;

	com::Buffer new_buf(buf_len, buf_len);

	// Build frame header
	prot::AudioFrameHdr* frame_hdr = (prot::AudioFrameHdr*)(DP(new_buf));
	frame_hdr->ver   = 0;
	frame_hdr->ext   = 0;
	frame_hdr->codec = (uint8_t)para->codec;
	frame_hdr->srate = media::util::ToProtAudioSRate(para->srate);
	frame_hdr->chnls = media::util::ToProtAudioChnls(para->chnls);
	frame_hdr->power = para->power;
	frame_hdr->fseq  = para->seq;
	frame_hdr->ts    = para->ts;

	memcpy(DP(new_buf) + hdr_len, DP(buf), buf.data_len);

	return new_buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer AudioSendElement::BuildStreamDataMsg(const com::Buffer& buf)
{
	uint32_t hdr_len = sizeof(prot::SigMsgHdr);
	uint32_t buf_len = buf.data_len + hdr_len;

	com::Buffer new_buf(buf_len, buf_len);

	// FIXME: 其他参数没有设置
	prot::SigMsgHdr* prot_hdr = (prot::SigMsgHdr*)(DP(new_buf));
	prot_hdr->len = (uint16_t)(buf.data_len);
	prot_hdr->mt = prot::MSG_STREAM_DATA;
	prot_hdr->seq = ++m_cur_seq;

	memcpy(DP(new_buf) + hdr_len, DP(buf), buf.data_len);

	return new_buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioSendElement::OnSinkPinData(ISinkPin* pin, const PinData& data)
{
	if (m_ele_state != EleState::RUNNING) {
		LOG_ERR("Element is not running!");
		return ERR_CODE_FAILED;
	}

	m_data_stats->OnData(m_br_stats, data.media_data->data_len);
	m_data_stats->OnData(m_bc_stats, data.media_data->data_len);
	m_data_stats->OnData(m_fc_stats, 1);

	if (m_stream_sender) {
		auto para = SPC<media::AudioFramePara>(data.media_para);
		com::Buffer frame_buf = BuildAudioFrameData(para, data.media_data[0]);
		m_stream_sender->InputFrameData(frame_buf);

		LOG_DBG("Send audio frame, seq:{}, len:{}", para->seq, frame_buf.data_len);
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode 
AudioSendElement::OnSinkPinNegotiated(ISinkPin* pin, const std::string& cap)
{
	LOG_INF("Sink pin negotiated, cap:{}", media::util::Capper(cap));

	auto sink_cap = media::util::ParseAudioCap(cap);
	if (!sink_cap.has_value()) {
		LOG_ERR("Parse audio cap failed!");
		return ERR_CODE_FAILED;
	}

	m_sink_pin_cap = sink_cap.value();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioSendElement::OnVideoStreamFeedback(const Buffer& buf)
{
	LOG_INF("Video stream feedback");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioSendElement::OnStartSendStreamNotify(const Buffer& buf)
{
	prot::StartSendStreamNotify notify;
	if (!notify.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse start send stream notify failed!");
		return;
	}

	// Start send stream
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioSendElement::OnStopSendStreamNotify(const Buffer& buf)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioSendElement::OnSessionData(const CommonMsg& msg)
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
void AudioSendElement::OnSessionClosed(const CommonMsg& msg)
{
	NotifyRunState("Audio send session closed");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioSendElement::OnLoginSendChannelRsp(const Buffer& buf)
{
	prot::LoginSendChannelRsp rsp;
	if (!rsp.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse register response failed!");
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
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioSendElement::OnLoginSendChannelTimeout()
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioSendElement::OnLoginSendChannelError(ErrCode ec)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioSendElement::OnSessionCreateResult(const CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(net::SessionCreateResultMsg);

	LOG_INF("Session create notify, lsid:{}, rsid:{}, result:{}", data->lsid,
		data->rsid, data->result);

	if (!data->result) {
		LOG_ERR("Session create failed");
		NotifyRunState("Audio send session create failed!");
		return;
	}
	NotifyRunState("Audio send session create success");

	prot::util::LoginSendChannelReqParam req_param;
	req_param.app_id    = m_stream_info.src.app_id;
	req_param.user_id   = m_stream_info.src.user_id;
	req_param.user_type = 0; // TODO:
	req_param.stream    = m_stream_info;
	req_param.token     = "unknown";

	com::SigHdrParam hdr_param;
	hdr_param.app_id  = m_stream_info.src.app_id;
	hdr_param.user_id = m_stream_info.src.user_id;
	hdr_param.seq     = ++m_cur_seq;

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

	LOG_INF("Send login send channel request, length:{}", buf.data_len);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioSendElement::OnThreadMsg(const CommonMsg& msg)
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
void AudioSendElement::OnStreamData(uint32_t channel_id, uint32_t user_id,
	const MediaStream& stream, const Buffer& buf)
{
	if (m_send_dumper) {
		m_send_dumper->WriteData(DP(buf), buf.data_len);
	}

	// 添加 SigMsgHdr
	com::Buffer sig_buf = BuildStreamDataMsg(buf);

	// 更新序列号统计
	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(sig_buf);
	m_data_stats->OnData(m_sn_stats, sig_hdr->seq);

	if (ERR_CODE_OK != m_sess_mgr->SendData(m_session_id, sig_buf)) {
		LOG_ERR("Send session data failed!");
	}

	if (!m_sending_data) {
		m_sending_data = true;
		NotifyRunState("Start sending audio data");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioSendElement::OnSenderFeedback(uint32_t channel_id, uint32_t user_id,
	const MediaStream& stream, const Buffer& buf)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioSendElement::OnEncoderTargetBitrate(uint32_t channel_id, 
	uint32_t user_id, const com::MediaStream& stream, uint32_t bw_kbps)
{

}

}