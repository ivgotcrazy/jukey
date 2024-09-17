#include "audio-convert-element.h"
#include "util-sdl.h"
#include "util-ffmpeg.h"
#include "util-streamer.h"
#include "common/util-common.h"
#include "pipeline-msg.h"
#include "log.h"
#include "common/media-common-define.h"
#include "yaml-cpp/yaml.h"


#define CONVERT_BUF_COUNT 8

using namespace jukey::com;
using namespace jukey::media::util;

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// media_type: video
//------------------------------------------------------------------------------
AudioConvertElement::AudioConvertElement(base::IComFactory* factory,
	const char* owner)
	: ProxyUnknown(nullptr)
	, ComObjTracer(factory, CID_AUDIO_CONVERT, owner)
	, ElementBase(factory)
{
	m_ele_name   = CONSTRUCT_ELEMENT_NAME("a-convert-");
	m_main_type  = EleMainType::FILTER;
	m_sub_type   = EleSubType::CONVERTER;
	m_media_type = EleMediaType::AUDIO_ONLY;

	if (ERR_CODE_OK != CreateSrcPin() || ERR_CODE_OK != CreateSinkPin()) {
		m_pins_created = false;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AudioConvertElement::~AudioConvertElement()
{
	LOG_INF("Destruct {}", m_ele_name);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* AudioConvertElement::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_AUDIO_CONVERT) == 0) {
		return new AudioConvertElement(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* AudioConvertElement::NDQueryInterface(const char* riid)
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
ErrCode AudioConvertElement::CreateSrcPin()
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
ErrCode AudioConvertElement::CreateSinkPin()
{
	media::com::AudioCaps sink_pin_caps;
	sink_pin_caps.AddCap(media::AudioCodec::PCM);
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
void AudioConvertElement::ParseElementConfig()
{
	try {
		YAML::Node root = YAML::LoadFile(ELEMENT_CONFIG_FILE);

		auto node = root["audio-convert-element"];
		if (!node) {
			return;
		}

		if (node["dump-before-convert"]) {
			if (node["dump-before-convert"].as<bool>()) {
				m_bc_dumper.reset(new StreamDumper(m_ele_name + "-bc.data"));
			}
		}

		if (node["dump-after-convert"]) {
			if (node["dump-after-convert"].as<bool>()) {
				m_ac_dumper.reset(new StreamDumper(m_ele_name + "-ac.data"));
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
ErrCode AudioConvertElement::DoInit(com::IProperty* props)
{
	m_logger = g_logger;

	ParseElementConfig();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioConvertElement::TryCreateSwrContext()
{
	if (!SRC_PIN->Negotiated() || !SINK_PIN->Negotiated()) {
		LOG_INF("Pin not negotiate");
		return ERR_CODE_OK;
	}

	m_need_convert = !(m_src_pin_cap == m_sink_pin_cap);

	if (m_need_convert) {
		return CreateSwrContext();
	}
	else {
		LOG_INF("Need not convert");
		return ERR_CODE_OK;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioConvertElement::DoStart()
{
	LOG_INF("DoStart");

	return TryCreateSwrContext();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioConvertElement::DoPause()
{
	LOG_INF("DoPause");

	DestroySwrContext();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioConvertElement::DoResume()
{
	LOG_INF("DoResume");

	if (ERR_CODE_OK != CreateSwrContext()) {
		LOG_ERR("Create swr context failed!");
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioConvertElement::DoStop()
{
	LOG_INF("DoStop");

	DestroySwrContext();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioConvertElement::ConvertPinData(const PinData& data)
{
	uint8_t* in_buf[CONVERT_BUF_COUNT] = { 0 };
	for (auto i = 0; i < CONVERT_BUF_COUNT; i++) {
		in_buf[i] = DP(data.media_data[i]);
	}

	uint8_t* out_buf[CONVERT_BUF_COUNT] = { 0 };
	if (media::util::IsPlanar(m_src_pin_cap.sbits)) {
		out_buf[0] = new uint8_t[1024 * 16];
		out_buf[1] = new uint8_t[1024 * 16];
	}
	else { // Packed
		out_buf[0] = new uint8_t[1024 * 16];
	}

	if (!data.media_para) {
		LOG_ERR("Invalid media parameters!");
		return ERR_CODE_INVALID_PARAM;
	}

	auto para = SPC<media::AudioFramePara>(data.media_para);

	int ret = swr_convert(m_swr_ctx, out_buf, 4096,
		(const uint8_t**)in_buf, para->count);
	if (ret <= 0) {
		LOG_ERR("swr_convert failed, ret:{}", ret);
		return ERR_CODE_FAILED;
	}

	// Get the number of bytes per sample of the output audio sample format
	int sample_bytes = av_get_bytes_per_sample(
		ToFfSampleFormat(m_src_pin_cap.sbits));
	
	// Successful converted sample count
	int num_samples = ret;
	
	// Get channel count
	int out_channels = GetChnlsNum(m_src_pin_cap.chnls);
	
	// Calculate the total number of bytes of the resampled data
	int data_size = num_samples * out_channels * sample_bytes;

	PinData new_data(media::MediaType::AUDIO);

	// Properties
	new_data.dts = data.dts;
	new_data.pts = data.pts;
	new_data.drt = data.drt;
	new_data.syn = data.syn;
	new_data.tbn = data.tbn;
	new_data.tbd = data.tbd;

	// Stereo && planar
	if (IsPlanar(m_src_pin_cap.sbits) &&
		m_src_pin_cap.chnls == media::AudioChnls::STEREO) {
		new_data.media_data[0].data.reset(out_buf[0]);
		new_data.media_data[0].data_len = data_size / 2;
		new_data.media_data[0].total_len = data_size / 2;

		new_data.media_data[1].data.reset(out_buf[1]);
		new_data.media_data[1].data_len = data_size / 2;
		new_data.media_data[1].total_len = data_size / 2;

		new_data.data_count = 2;
	}
	else {
		new_data.media_data[0].data.reset(out_buf[0]);
		new_data.media_data[0].data_len = data_size;
		new_data.media_data[0].total_len = data_size;
		new_data.data_count = 1;
	}

	// Copy parameters
	new_data.media_para = data.media_para;

	if (m_ac_dumper) {
		m_ac_dumper->WriteStreamData(new_data);
	}

	return SRC_PIN->OnPinData(new_data);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioConvertElement::OnSinkPinData(ISinkPin* pin, const PinData& data)
{
	LOG_DBG("Sin pin data, len:{}", data.media_data[0].data_len);

	if (m_ele_state != EleState::RUNNING) {
		LOG_DBG("Invalid element state:{}!", m_ele_state);
		return ERR_CODE_FAILED;
	}

	assert(data.mt == media::MediaType::AUDIO);

	if (m_bc_dumper) {
		m_bc_dumper->WriteStreamData(data);
	}

	if (m_need_convert) {
		if (m_swr_ctx) {
			return ConvertPinData(data);
		}
		else {
			LOG_ERR("Invalid SwrContext!!!");
			return ERR_CODE_OK;
		}
	}
	else {
		return SRC_PIN->OnPinData(data);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioConvertElement::UpdatePinCap(const std::string& src_cap,
	const std::string& sink_cap)
{
	if (!src_cap.empty()) {
		auto audio_src_cap = media::util::ParseAudioCap(src_cap);
		if (audio_src_cap.has_value()) {
			m_src_pin_cap = audio_src_cap.value();
		}
		else {
			LOG_ERR("Parse cap failed, cap:{}", src_cap);
		}
	}
	
	if (!sink_cap.empty()) {
		auto audio_sink_cap = media::util::ParseAudioCap(sink_cap);
		if (audio_sink_cap.has_value()) {
			m_sink_pin_cap = audio_sink_cap.value();
		}
		else {
			LOG_ERR("Parse cap failed, cap:{}", sink_cap);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioConvertElement::OnSinkPinNegotiated(ISinkPin* pin, 
	const std::string& cap)
{
	LOG_INF("Sink pin negotiated, cap:{}", media::util::Capper(cap));

	if (ERR_CODE_OK != SRC_PIN->Negotiate()) {
		LOG_ERR("Src pin negotiate failed");
		return ERR_CODE_FAILED;
	}

	LOG_INF("Src pin negotiated cap:{}", media::util::Capper(SRC_PIN->Cap()));

	UpdatePinCap(SRC_PIN->Cap(), cap);

	return TryCreateSwrContext();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode 
AudioConvertElement::OnSrcPinNegotiated(ISrcPin* pin, const std::string& cap)
{
	LOG_INF("Src pin negotiated, cap:{}", media::util::Capper(cap));

	if (!pin) {
		LOG_ERR("Invalid pin");
		return ERR_CODE_INVALID_PARAM;
	}

	UpdatePinCap(cap, SINK_PIN->Cap());

	return TryCreateSwrContext();
}

//------------------------------------------------------------------------------
// TODO: Repeat negotiate
//------------------------------------------------------------------------------
ErrCode AudioConvertElement::CreateSwrContext()
{
	int64_t out_channel_layout = av_get_default_channel_layout(
		(int)m_src_pin_cap.chnls);

	AVSampleFormat out_sample_fmt = 
		media::util::ToFfSampleFormat(m_src_pin_cap.sbits);

	int out_sample_rate = media::util::GetSRateNum(m_src_pin_cap.srate);

	int64_t in_channel_layout = av_get_default_channel_layout(
		(int)m_sink_pin_cap.chnls);

	AVSampleFormat in_sample_fmt = 
		media::util::ToFfSampleFormat(m_sink_pin_cap.sbits);

	int in_sample_rate = media::util::GetSRateNum(m_sink_pin_cap.srate);

	m_swr_ctx = swr_alloc_set_opts(NULL,
		out_channel_layout,
		out_sample_fmt,
		out_sample_rate,
		in_channel_layout,
		in_sample_fmt,
		in_sample_rate,
		0,
		NULL);
	if (!m_swr_ctx) {
		LOG_ERR("swr_alloc_set_opts failed!");
		return ERR_CODE_FAILED;
	}

	int result = swr_init(m_swr_ctx);
	if (result != 0) {
		LOG_ERR("swr_init failed, error:{}", result);
		return ERR_CODE_FAILED;
	}

	LOG_INF("Create SWR context success");

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioConvertElement::DestroySwrContext()
{
	if (m_swr_ctx) {
		swr_free(&m_swr_ctx);
		m_swr_ctx = nullptr;
	}
}

}