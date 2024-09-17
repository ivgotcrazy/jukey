#include "audio-recv-element.h"
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


using namespace jukey::util;
using namespace jukey::com;

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AudioRecvElement::AudioRecvElement(base::IComFactory* factory, const char* owner)
	: ProxyUnknown(nullptr)
	, ComObjTracer(factory, CID_AUDIO_RECV, owner)
	, ElementBase(factory)
	, CommonThread("Audio recv element", false)
{
	m_ele_name   = CONSTRUCT_ELEMENT_NAME("a-recv-");
	m_main_type  = EleMainType::SRC;
	m_sub_type   = EleSubType::RECEIVER;
	m_media_type = EleMediaType::AUDIO_ONLY;

	if (ERR_CODE_OK != CreateSrcPin()) {
		m_pins_created = false;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AudioRecvElement::~AudioRecvElement()
{
	LOG_INF("Destruct {}", m_ele_name);

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
base::IUnknown* AudioRecvElement::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_AUDIO_RECV) == 0) {
		return new AudioRecvElement(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* AudioRecvElement::NDQueryInterface(const char* riid)
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
ErrCode AudioRecvElement::CreateSrcPin()
{
	media::com::AudioCaps src_pin_caps;
	src_pin_caps.AddCap(media::AudioCodec::OPUS);
	src_pin_caps.AddCap(media::AudioChnls::STEREO);
	//src_pin_caps.AddCap(media::AudioChnls::MONO);
	src_pin_caps.AddCap(media::AudioSBits::S16);
	//src_pin_caps.AddCap(media::AudioSBits::S32);
	//src_pin_caps.AddCap(media::AudioSBits::S16P);
	//src_pin_caps.AddCap(media::AudioSBits::S32P);
	//src_pin_caps.AddCap(media::AudioSBits::FLTP);
	src_pin_caps.AddCap(media::AudioSRate::SR_48K);
	//src_pin_caps.AddCap(media::AudioSRate::SR_16K);

	ISrcPin* src_pin = (ISrcPin*)QI(CID_SRC_PIN, IID_SRC_PIN, m_ele_name);
	if (!src_pin) {
		LOG_ERR("Create src pin failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != src_pin->Init(media::MediaType::AUDIO,
			this,
			CONSTRUCT_PIN_NAME("a-src-pin-"),
			media::util::ToAudioCapsStr(src_pin_caps),
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
ErrCode AudioRecvElement::ParseProperties(com::IProperty* props)
{
	m_sess_mgr = (net::ISessionMgr*)props->GetPtrValue("session-mgr");
	if (!m_sess_mgr) {
		LOG_ERR("Cannot find session-mgr property!");
		return ERR_CODE_FAILED;
	}

	const void* info = props->GetPtrValue("media-stream");
	if (!info) {
		LOG_ERR("Invalid stream-id property!");
		return ERR_CODE_OK;
	}
	m_stream_info = *((com::MediaStream*)info);

	LOG_INF("Read property stream-info, stream_id:{}",
		m_stream_info.stream.stream_id);

	const char* addr = props->GetStrValue("service-addr");
	if (!addr) {
		LOG_ERR("Cannot find addr property!");
		return ERR_CODE_FAILED;
	}

	LOG_INF("Read property service-addr:{}", addr);

	std::optional<com::Address> o = ParseAddress(addr);
	if (o.has_value()) {
		m_service_addr = o.value();
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
void AudioRecvElement::ParseElementConfig()
{
	try {
		YAML::Node root = YAML::LoadFile(ELEMENT_CONFIG_FILE);

		auto node = root["audio-recv-element"];
		if (!node) {
			return;
		}

		if (node["dump-recv-data"]) {
			if (node["dump-recv-data"].as<bool>()) {
				m_recv_dumper.reset(new util::DataDumper(m_ele_name + ".data"));
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
ErrCode AudioRecvElement::DoInit(com::IProperty* props)
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

	StatsParam bc_stats("bitcount", StatsType::IACCU, 5000);
	m_bc_stats = m_data_stats->AddStats(bc_stats);

	StatsParam fc_stats("frame-count", StatsType::IACCU, 5000);
	m_fc_stats = m_data_stats->AddStats(fc_stats);

	StatsParam sn_stats("sn", StatsType::ISNAP, 5000);
	m_sn_stats = m_data_stats->AddStats(sn_stats);

	m_async_proxy.reset(new util::SessionAsyncProxy(m_factory, m_sess_mgr,
		this, 10000));

	m_pipeline->SubscribeMsg(PlMsgType::NEGOTIATE, this);

	StartThread();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioRecvElement::DoStart()
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
void AudioRecvElement::OnAudioStreamData(const Buffer& buf)
{
	LOG_DBG("OnAudioStreamData, data len:{}", buf.data_len);

	if (m_recv_dumper) {
		m_recv_dumper->WriteData(DP(buf), buf.data_len);
	}

	m_stream_receiver->InputStreamData(buf);

	if (!m_receiving_data) {
		m_receiving_data = true;
		NotifyRunState("Start receiving audio data");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioRecvElement::OnAudioStreamFeedback(const Buffer& buf)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioRecvElement::OnPauseRecvStreamRsp(const Buffer& buf)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioRecvElement::OnResumeRecvStreamRsp(const Buffer& buf)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioRecvElement::OnSessionData(const CommonMsg& msg)
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
		OnAudioStreamData(data->buf);
		break;
	case prot::MSG_STREAM_FEEDBACK:
		OnAudioStreamFeedback(data->buf);
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
void AudioRecvElement::OnSessionClosed(const CommonMsg& msg)
{
	NotifyRunState("Audio receive session closed!");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioRecvElement::OnLoginRecvChannelRsp(const Buffer& buf)
{
	prot::LoginRecvChannelRsp rsp;
	if (!rsp.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse register response failed!");
		return;
	}

	LOG_INF("Received login recv channel response:{}", util::PbMsgToJson(rsp));

	m_stream_receiver = (txp::IStreamReceiver*)QI(CID_STREAM_RECEIVER,
		IID_STREAM_RECEIVER, m_ele_name);
	if (!m_stream_receiver) {
		LOG_ERR("Create stream receiver failed!");
		return;
	}

	if (ERR_CODE_OK != m_stream_receiver->Init(this, rsp.channel_id(),
		rsp.user_id(), m_stream_info)) {
		LOG_ERR("Initialize stream sender failed!");
		return;
	}

	// Notify stream

	std::string stream_id = m_stream_info.stream.stream_id;

	if (ERR_CODE_OK != SRC_PIN->OnPinMsg(nullptr,
		PinMsg(PinMsgType::SET_STREAM, stream_id))) {
		LOG_ERR("Set src pin:{} stream failed!", SRC_PIN->Name());
		return;
	}

	ElementStream* es = new ElementStream();
	es->stream.src = m_stream_info.src;
	es->stream.stream.stream_type = StreamType::AUDIO;
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
void AudioRecvElement::OnLoginRecvChannelTimeout()
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioRecvElement::OnLoginRecvChannelError(ErrCode ec)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioRecvElement::OnSessionCreateResult(const CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(net::SessionCreateResultMsg);

	LOG_INF("Session create notify, lsid:{}, rsid:{}, result:{}",
		data->lsid, data->rsid, data->result);

	if (!data->result) {
		LOG_ERR("Create session failed!");
		NotifyRunState("Audio receive session create failed!");
		return;
	}
	NotifyRunState("Audio receive session create success");

	prot::util::LoginRecvChannelReqParam req_param;
	req_param.app_id    = m_stream_info.src.app_id;
	req_param.user_id   = m_stream_info.src.user_id;
	req_param.user_type = 0; // TODO:
	req_param.stream    = m_stream_info;
	req_param.token     = "unknown";

	com::SigHdrParam hdr_param;
	hdr_param.app_id  = m_stream_info.src.app_id;
	hdr_param.user_id = m_stream_info.src.user_id;
	hdr_param.seq     = ++m_cur_seq;


	// Login receive channel
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

	LOG_INF("Send login recv channel request, length:{}", buf.data_len);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioRecvElement::OnThreadMsg(const CommonMsg& msg)
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
// TODO: fixed sbits
//------------------------------------------------------------------------------
void AudioRecvElement::UpdateCaps(media::AudioChnls chnls, media::AudioSRate srate)
{
	media::com::AudioCap audio_cap;
	audio_cap.codec = media::AudioCodec::OPUS;
	audio_cap.chnls = chnls;
	audio_cap.sbits = media::AudioSBits::S16;
	audio_cap.srate = srate;

	std::vector<media::com::AudioCap> audio_caps;
	audio_caps.push_back(audio_cap);

	PinCaps avai_caps = media::util::ToPinCaps(audio_caps);

	if (ERR_CODE_OK != SRC_PIN->UpdateAvaiCaps(avai_caps)) {
		LOG_ERR("Update src pin available caps failed");
		return;
	}

	// Record last received audio channels and sample rate
	m_last_chnls = chnls;
	m_last_srate = srate;

	if (ERR_CODE_OK != SRC_PIN->Negotiate()) {
		LOG_ERR("Negotiate failed");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioRecvElement::OnStreamFrame(uint32_t channel_id, uint32_t user_id,
	const com::MediaStream& stream, const com::Buffer& buf)
{
	prot::AudioFrameHdr* hdr = (prot::AudioFrameHdr*)DP(buf);

	m_data_stats->OnData(m_fc_stats, 1);
	m_data_stats->OnData(m_br_stats, buf.data_len);
	m_data_stats->OnData(m_bc_stats, buf.data_len);
	m_data_stats->OnData(m_sn_stats, hdr->fseq);

	PinData pin_data(media::MediaType::AUDIO);

	pin_data.dts = hdr->ts;
	pin_data.pts = hdr->ts;
	pin_data.pos = 0;
	pin_data.drt = 0;
	pin_data.pos = 0;
	pin_data.syn = 0;
	pin_data.tbn = 0;
	pin_data.tbd = 0;

	pin_data.data_count = 1;
	//pin_data.media_para = audio_frame.frame_para;

	com::Buffer& audio_buf = const_cast<com::Buffer&>(buf);
	audio_buf.start_pos += sizeof(prot::AudioFrameHdr);
	audio_buf.data_len -= sizeof(prot::AudioFrameHdr);

	pin_data.media_data[0] = audio_buf;

	media::AudioChnls curr_chnls = (media::AudioChnls)hdr->chnls;
	media::AudioSRate curr_srate = (media::AudioSRate)(hdr->srate + 2);

	if (curr_chnls >= media::AudioChnls::BUTT) {
		LOG_ERR("Invalid audio channels:{}", curr_chnls);
		return;
	}

	if (curr_srate >= media::AudioSRate::BUTT) {
		LOG_ERR("Invalid audio sample rate:{}", curr_srate);
		return;
	}

	//if (curr_chnls != m_last_chnls || curr_srate != m_last_srate) {
	//	UpdateCaps(curr_chnls, curr_srate);
	//}

	if (m_ele_state == EleState::RUNNING) {
		SRC_PIN->OnPinData(pin_data);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioRecvElement::OnReceiverFeedback(uint32_t channel_id,
	uint32_t user_id, const com::MediaStream& stream, const com::Buffer& buf)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioRecvElement::OnPipelineNegotiateMsg(const com::CommonMsg& msg)
{
	if (ERR_CODE_OK != SRC_PIN->Negotiate()) {
		LOG_ERR("Src pin negotiate failed");
		if (msg.result) {
			msg.result->set_value(ERR_CODE_FAILED);
		}
	}
	else {
		if (msg.result) {
			msg.result->set_value(ERR_CODE_OK);
		}
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioRecvElement::PreProcPipelineMsg(const CommonMsg& msg)
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
ErrCode AudioRecvElement::OnSrcPinNegotiated(ISrcPin* pin, const std::string& cap)
{
	LOG_INF("Src pin negotiated, cap:{}", media::util::Capper(cap));

	auto src_cap = media::util::ParseAudioCap(cap);
	if (!src_cap.has_value()) {
		LOG_ERR("Parse audio cap failed");
		return ERR_CODE_FAILED;
	}

	m_src_pin_cap = src_cap.value();

	return ERR_CODE_OK;
}

}