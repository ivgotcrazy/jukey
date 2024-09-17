#include "audio-decode-element.h"
#include "util-streamer.h"
#include "util-ffmpeg.h"
#include "util-streamer.h"
#include "common/util-common.h"
#include "pipeline-msg.h"
#include "log.h"
#include "common/media-common-define.h"
#include "yaml-cpp/yaml.h"


using namespace jukey::com;
using namespace jukey::media::util;

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AudioDecodeElement::AudioDecodeElement(base::IComFactory* factory,
	const char* owner)
	: base::ProxyUnknown(nullptr)
  , base::ComObjTracer(factory, CID_AUDIO_DECODE, owner)
	, ElementBase(factory)
{
	m_ele_name   = CONSTRUCT_ELEMENT_NAME("a-decode-");
	m_main_type  = EleMainType::FILTER;
	m_sub_type   = EleSubType::DECODER;
	m_media_type = EleMediaType::AUDIO_ONLY;

	if (ERR_CODE_OK != CreateSrcPin() || ERR_CODE_OK != CreateSinkPin()) {
		m_pins_created = false;
	}

	m_frame = av_frame_alloc();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AudioDecodeElement::~AudioDecodeElement()
{
	LOG_INF("Destruct {}", m_ele_name);

	if (m_frame) {
		av_frame_free(&m_frame);
		m_frame = nullptr;
	}

	if (m_opus_decoder) {
		opus_decoder_destroy(m_opus_decoder);
		m_opus_decoder = nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* AudioDecodeElement::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_AUDIO_DECODE) == 0) {
		return new AudioDecodeElement(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* AudioDecodeElement::NDQueryInterface(const char* riid)
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
ErrCode AudioDecodeElement::CreateSrcPin()
{
	media::com::AudioCaps src_pin_caps;
	src_pin_caps.AddCap(media::AudioCodec::PCM);
	src_pin_caps.AddCap(media::AudioChnls::STEREO);
	src_pin_caps.AddCap(media::AudioChnls::MONO);
	src_pin_caps.AddCap(media::AudioSBits::S16);
	src_pin_caps.AddCap(media::AudioSBits::S32);
	src_pin_caps.AddCap(media::AudioSBits::S16P);
	src_pin_caps.AddCap(media::AudioSBits::S32P);
	src_pin_caps.AddCap(media::AudioSBits::FLTP);
	src_pin_caps.AddCap(media::AudioSRate::SR_48K);
	src_pin_caps.AddCap(media::AudioSRate::SR_16K);

	ISrcPin* src_pin = (ISrcPin*)QI(CID_SRC_PIN, IID_SRC_PIN, m_ele_name);
	if (!src_pin) {
		LOG_ERR("Create src pin failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != src_pin->Init(media::MediaType::AUDIO,
			this,
			CONSTRUCT_PIN_NAME("a-src-pin-"),
			ToAudioCapsStr(src_pin_caps),
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
ErrCode AudioDecodeElement::CreateSinkPin()
{
	media::com::AudioCaps sink_pin_caps;
	sink_pin_caps.AddCap(media::AudioCodec::OPUS);
	sink_pin_caps.AddCap(media::AudioCodec::AAC);
	sink_pin_caps.AddCap(media::AudioChnls::STEREO);
	sink_pin_caps.AddCap(media::AudioChnls::MONO);
	sink_pin_caps.AddCap(media::AudioSBits::S16);
	sink_pin_caps.AddCap(media::AudioSBits::S32);
	sink_pin_caps.AddCap(media::AudioSBits::S16P);
	sink_pin_caps.AddCap(media::AudioSBits::S32P);
	sink_pin_caps.AddCap(media::AudioSBits::FLTP);
	sink_pin_caps.AddCap(media::AudioSRate::SR_48K);
	sink_pin_caps.AddCap(media::AudioSRate::SR_16K);

	ISinkPin* sink_pin = (ISinkPin*)QI(CID_SINK_PIN, IID_SINK_PIN, m_ele_name);
	if (!sink_pin) {
		LOG_ERR("Create sink pin failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != sink_pin->Init(media::MediaType::AUDIO,
			this,
			CONSTRUCT_PIN_NAME("a-sink-pin-"),
			ToAudioCapsStr(sink_pin_caps),
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
void AudioDecodeElement::ParseElementConfig()
{
	try {
		YAML::Node root = YAML::LoadFile(ELEMENT_CONFIG_FILE);

		auto node = root["audio-decode-element"];
		if (!node) {
			return;
		}

		if (node["dump-before-decode"]) {
			if (node["dump-before-decode"].as<bool>()) {
				m_bd_dumper.reset(new StreamDumper(m_ele_name + "-bd.data"));
			}
		}

		if (node["dump-after-decode"]) {
			if (node["dump-after-decode"].as<bool>()) {
				m_ad_dumper.reset(new StreamDumper(m_ele_name + "-ad.data"));
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
ErrCode AudioDecodeElement::DoInit(com::IProperty* props)
{
	LOG_INF("DoInit");

	m_logger = g_logger;

	av_log_set_callback(LogCallback);

	ParseElementConfig();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioDecodeElement::DoStart()
{
	LOG_INF("DoStart");

	if (ERR_CODE_OK != CreateDecoder()) {
		LOG_ERR("Create decoder failed!");
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioDecodeElement::DoPause()
{
	LOG_INF("DoPause");

	DestroyDecoder();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioDecodeElement::DoResume()
{
	LOG_INF("DoResume");

	if (ERR_CODE_OK != CreateDecoder()) {
		LOG_ERR("Create decoder failed!");
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioDecodeElement::DoStop()
{
	LOG_INF("DoStop");

	DestroyDecoder();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioDecodeElement::CreateFfmpegDecoder()
{
	m_codec = avcodec_find_decoder(ToFfAudioCodec(m_sink_pin_cap.codec));
	if (!m_codec) {
		LOG_ERR("Create avcodec failed!");
		return ERR_CODE_FAILED;
	}

	m_codec_ctx = avcodec_alloc_context3(m_codec);
	if (!m_codec_ctx) {
		LOG_ERR("Create avcodec context failed!");
		return ERR_CODE_FAILED;
	}

	m_codec_ctx->channels    = GetChnlsNum(m_sink_pin_cap.chnls);
	m_codec_ctx->sample_fmt  = ToFfSampleFormat(m_sink_pin_cap.sbits);
	m_codec_ctx->sample_rate = GetSRateNum(m_sink_pin_cap.srate);
	//m_codec_ctx->frame_size = 960;
	//m_codec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;

	if (avcodec_open2(m_codec_ctx, m_codec, NULL) < 0) {
		LOG_ERR("Open codec:{} failed!", m_sink_pin_cap.codec);
		return ERR_CODE_FAILED;
	}

	LOG_INF("Create ffmpeg decoder success");

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioDecodeElement::CreateOpusDecoder()
{
	int error = 0;
	m_opus_decoder = opus_decoder_create(GetSRateNum(m_sink_pin_cap.srate),
		GetChnlsNum(m_sink_pin_cap.chnls), &error);
	if (!m_opus_decoder) {
		LOG_ERR("Create opus decoder failed, error:{}", error);
		return ERR_CODE_FAILED;
	}

	LOG_INF("Create opus decoder success, sample rate:{}, channles:{}",
		m_sink_pin_cap.srate, m_sink_pin_cap.chnls);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioDecodeElement::DoFfmpegDecode(const PinData& data)
{
	if (m_bd_dumper) {
		m_bd_dumper->WriteStreamData(data);
	}

	ErrCode result = ERR_CODE_OK;

	AVPacket packet = { 0 };
	packet.data = (uint8_t*)DP(data.media_data[0]);
	packet.size = data.media_data[0].data_len;
	packet.dts  = data.dts;
	packet.pts  = data.pts;
	packet.pos  = data.pos;
	packet.duration = data.drt;

	int ret = avcodec_send_packet(m_codec_ctx, &packet);
	if (ret != 0) {
		if (ret == AVERROR(EAGAIN)) {
			LOG_WRN("EAGAIN");
		}
		else {
			LOG_ERR("avcodec_send_packet failed, ret:{}", ret);
			goto EXIT;
		}
	}

	while (true) {
		int ret = avcodec_receive_frame(m_codec_ctx, m_frame);
		if (ret != 0) {
			if (ret != AVERROR(EAGAIN)) {
				LOG_WRN("Receive frame failed, error:{}", ret);
			}
			break;
		}

		PinData decode_data(media::MediaType::AUDIO);

		// Properties
		decode_data.dts = m_frame->pkt_dts;
		decode_data.pts = m_frame->pts;
		decode_data.drt = m_frame->pkt_duration;
		decode_data.pos = m_frame->pkt_pos;
		decode_data.syn = data.syn;
		decode_data.tbn = data.tbn;
		decode_data.tbd = data.tbd;

		// Data
		decode_data.media_data[0].data.reset(m_frame->data[0], util::NoDestruct);
		decode_data.media_data[0].data_len = m_frame->linesize[0] / 2;
		decode_data.media_data[1].data.reset(m_frame->data[1], util::NoDestruct);
		decode_data.media_data[1].data_len = m_frame->linesize[0] / 2;
		decode_data.data_count = 2;

		LOG_DBG("after decode data len:{}", m_frame->linesize[0]);

		if (m_ad_dumper) {
			m_ad_dumper->WriteStreamData(decode_data);
		}

		// Parameters
		auto para = SPC<media::AudioFramePara>(decode_data.media_para);
		para->count = m_frame->nb_samples;

		result = SRC_PIN->OnPinData(decode_data);

		av_frame_unref(m_frame);
	}

EXIT:
	av_packet_unref(&packet);

	return result;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioDecodeElement::DoOpusDecode(const PinData& data)
{
	if (m_bd_dumper) {
		m_bd_dumper->WriteData(DP(data.media_data[0]), data.media_data[0].data_len);
	}
	
	if (!m_opus_decoder) {
		LOG_ERR("Invalid opus decoder");
		return ERR_CODE_FAILED;
	}

	PinData decoded_data(media::MediaType::AUDIO, 1024 * 32);
	decoded_data.pts = data.pts;
	decoded_data.dts = data.dts;
	decoded_data.syn = data.syn;
	decoded_data.tbd = data.tbd;
	decoded_data.tbn = data.tbn;

	int samples = opus_decode(m_opus_decoder, 
		DP(data.media_data[0]),
		data.media_data[0].data_len,
		(opus_int16*)DP(decoded_data.media_data[0]), 
		8192,
		0);
	if (samples <= 0) {
		LOG_ERR("opus_decode failed, result:{}", samples);
		return ERR_CODE_FAILED;
	}

	// samples * channles * sample bits
	decoded_data.media_data[0].data_len = samples * 2 * 2;

	if (m_ad_dumper) {
		m_ad_dumper->WriteData(DP(decoded_data.media_data[0]),
			decoded_data.media_data[0].data_len);
	}

	return SRC_PIN->OnPinData(decoded_data);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioDecodeElement::OnSinkPinData(ISinkPin* pin, const PinData& data)
{
	LOG_DBG("Received decode data:{}", data.media_data->data_len);

	if (m_ele_state != EleState::RUNNING) {
		LOG_ERR("Element is not running!");
		return ERR_CODE_FAILED;
	}

	if (m_use_ffmpeg) {
		return DoFfmpegDecode(data);
	}
	else {
		return DoOpusDecode(data);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioDecodeElement::UpdatePinCap(const std::string& src_cap,
	const std::string& sink_cap)
{
	if (!src_cap.empty()) {
		auto audio_src_cap = ParseAudioCap(src_cap);
		if (audio_src_cap.has_value()) {
			m_src_pin_cap = audio_src_cap.value();
		}
		else {
			LOG_ERR("Parse src cap failed, cap:{}", src_cap);
		}
	}

	if (!sink_cap.empty()) {
		auto audio_sink_cap = ParseAudioCap(sink_cap);
		if (audio_sink_cap.has_value()) {
			m_sink_pin_cap = audio_sink_cap.value();
		}
		else {
			LOG_ERR("Parse sink cap failed, cap:{}", sink_cap);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioDecodeElement::OnSinkPinNegotiated(ISinkPin* pin,
	const std::string& cap)
{
	LOG_INF("Sink pin negotiated, cap:{}", Capper(cap));

	if (!pin) {
		LOG_ERR("Invalid pin");
		return ERR_CODE_FAILED;
	}

	if (SRC_PIN->PrepCaps().empty()) {
		LOG_ERR("Empty src pin prepared caps");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != SRC_PIN->UpdateAvaiCaps(
		MatchAudioPinCapsWithoutCodec(SRC_PIN->PrepCaps(), cap))) {
		LOG_ERR("Update src pin available caps failed");
		return ERR_CODE_FAILED;
	}
	
	if (ERR_CODE_OK != SRC_PIN->Negotiate()) {
		LOG_ERR("Src pin negotiate failed");
		return ERR_CODE_FAILED;
	}

	LOG_INF("Src pin negotiated cap:{}", Capper(SRC_PIN->Cap()));

	UpdatePinCap(SRC_PIN->Cap(), cap);
	CreateDecoder();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode 
AudioDecodeElement::OnSrcPinNegotiated(ISrcPin* pin, const std::string& cap)
{
	LOG_INF("src pin negotiated, cap:{}", media::util::Capper(cap));

	if (!pin) {
		LOG_ERR("Invalid pin");
		return ERR_CODE_INVALID_PARAM;
	}

	UpdatePinCap(cap, SINK_PIN->Cap());
	CreateDecoder();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// TODO: repeatedly create
//------------------------------------------------------------------------------
ErrCode AudioDecodeElement::CreateDecoder()
{
	if (!SRC_PIN->Negotiated() || !SINK_PIN->Negotiated()) {
		LOG_INF("Element not negotiated!");
		return ERR_CODE_OK;
	}

	auto audio_cap = media::util::ParseAudioCap(SINK_PIN->Cap());
	if (!audio_cap.has_value()) {
		LOG_ERR("Parse audio cap failed, cap:{}", SINK_PIN->Cap());
		return ERR_CODE_FAILED;
	}

	m_use_ffmpeg = (audio_cap->codec != media::AudioCodec::OPUS);

	if (m_use_ffmpeg) {
		return CreateFfmpegDecoder();
	}
	else {
		return CreateOpusDecoder();
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioDecodeElement::DestroyDecoder()
{
	LOG_INF("Destroy decoder");

	if (m_codec_ctx) {
		avcodec_close(m_codec_ctx);
		avcodec_free_context(&m_codec_ctx);
		m_codec_ctx = nullptr;
	}
}

}