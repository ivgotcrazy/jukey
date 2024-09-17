#include <string.h>
#include "audio-encode-element.h"
#include "util-streamer.h"
#include "util-streamer.h"
#include "common/util-common.h"
#include "common-config.h"
#include "log.h"
#include "common/media-common-define.h"
#include "yaml-cpp/yaml.h"


using namespace jukey::com;

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AudioEncodeElement::AudioEncodeElement(base::IComFactory* factory,
	const char* owner)
	: base::ProxyUnknown(nullptr)
  , base::ComObjTracer(factory, CID_AUDIO_ENCODE, owner)
	, util::CommonThread("audio encode element", false)
  , media::util::ElementBase(factory)
{
	m_ele_name   = CONSTRUCT_ELEMENT_NAME("a-encode-");
	m_main_type  = EleMainType::FILTER;
	m_sub_type   = EleSubType::ENCODER;
	m_media_type = EleMediaType::AUDIO_ONLY;

	if (ERR_CODE_OK != CreateSrcPin() || ERR_CODE_OK != CreateSinkPin()) {
		m_pins_created = false;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AudioEncodeElement::~AudioEncodeElement()
{
	LOG_INF("Destruct {}", m_ele_name);

	StopThread();

	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_opus_encoder) {
		opus_encoder_destroy(m_opus_encoder);
		m_opus_encoder = nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* AudioEncodeElement::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_AUDIO_ENCODE) == 0) {
		return new AudioEncodeElement(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* AudioEncodeElement::NDQueryInterface(const char* riid)
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
ErrCode AudioEncodeElement::CreateSrcPin()
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
ErrCode AudioEncodeElement::CreateSinkPin()
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
			media::util::ToAudioCapsStr(sink_pin_caps),
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
void AudioEncodeElement::ParseElementConfig()
{
	try {
		YAML::Node root = YAML::LoadFile(ELEMENT_CONFIG_FILE);

		auto node = root["audio-encode-element"];
		if (!node) {
			return;
		}

		if (node["dump-before-encode"]) {
			if (node["dump-before-encode"].as<bool>()) {
				m_be_dumper.reset(new util::DataDumper(m_ele_name + "-be.data"));
			}
		}

		if (node["dump-after-encode"]) {
			if (node["dump-after-encode"].as<bool>()) {
				m_ae_dumper.reset(new util::DataDumper(m_ele_name + "-ae.data"));
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
ErrCode AudioEncodeElement::DoInit(com::IProperty* props)
{
	LOG_INF("DoInit");

	m_logger = g_logger;

	ParseElementConfig();

	StartThread();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioEncodeElement::DoStart()
{
	LOG_INF("DoStart");

	if (com::ERR_CODE_OK != CreateEncoder()) {
		LOG_ERR("Create encoder failed!");
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioEncodeElement::CreateEncoder()
{
	if (!SINK_PIN->Negotiated()) {
		LOG_WRN("Sink pin not negotiated, cannot create encoder!");
		return ERR_CODE_OK;
	}

	uint32_t srate = media::util::GetSRateNum(m_sink_pin_cap.srate);
	uint32_t chnls = media::util::GetChnlsNum(m_sink_pin_cap.chnls);

	int error;
	m_opus_encoder = opus_encoder_create(srate, chnls, OPUS_APPLICATION_AUDIO, 
		&error);
	if (!m_opus_encoder) {
		LOG_ERR("Create opus encoder failed, error:{}", error);
		return ERR_CODE_FAILED;
	}

	LOG_INF("Create opus encoder success, sample rate:{}, channles:{}",
		m_sink_pin_cap.srate, m_sink_pin_cap.chnls);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioEncodeElement::OnSinkPinData(ISinkPin* pin, const PinData& data)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_ele_state != EleState::RUNNING) {
		LOG_ERR("Element is not running!");
		return ERR_CODE_FAILED;
	}

	if (!m_opus_encoder) {
		LOG_ERR("Invalid opus encoder!");
		return ERR_CODE_INVALID_PARAM;
	}

	if (m_ele_state != EleState::RUNNING) {
		LOG_DBG("Element is not running!");
		return ERR_CODE_FAILED;
	}

	if (m_be_dumper) {
		m_be_dumper->WriteData(DP(data.media_data[0]), data.media_data[0].data_len);
	}

	m_data_que.push_back(media::util::ClonePinData(data));
	m_con_var.notify_all();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioEncodeElement::UpdatePinCap(const std::string& src_cap,
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
ErrCode 
AudioEncodeElement::OnSinkPinNegotiated(ISinkPin* pin, const std::string& cap)
{
	LOG_INF("Sink pin negotiated, cap:{}", media::util::Capper(cap));

	if (!pin) {
		LOG_ERR("Invalid pin");
		return ERR_CODE_INVALID_PARAM;
	}

	if (SRC_PIN->PrepCaps().empty()) {
		LOG_ERR("Empty src pin prepared caps");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != SRC_PIN->UpdateAvaiCaps(
		media::util::MatchAudioPinCapsWithoutCodec(SRC_PIN->PrepCaps(), cap))) {
		LOG_ERR("Update src pin available caps failed");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != SRC_PIN->Negotiate()) {
		LOG_ERR("Src pin negotiate failed");
		return ERR_CODE_FAILED;
	}

	UpdatePinCap(SRC_PIN->Cap(), cap);

	LOG_INF("Src pin negotiated cap:{}", media::util::Capper(SRC_PIN->Cap()));

	m_input_len = media::util::GetSRateNum(m_sink_pin_cap.srate)
		* media::util::GetSBitsNum(m_sink_pin_cap.sbits) / 8
		* media::util::GetChnlsNum(m_sink_pin_cap.chnls)
		* OPUS_PACK_DURATION / 1000;

	return CreateEncoder();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode 
AudioEncodeElement::OnSrcPinNegotiated(ISrcPin* pin, const std::string& cap)
{
	LOG_INF("Src pin negotiated, cap:{}", media::util::Capper(cap));

	if (!pin) {
		LOG_ERR("Invalid pin");
		return ERR_CODE_INVALID_PARAM;
	}

	UpdatePinCap(cap, SINK_PIN->Cap());

	return CreateEncoder();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t AudioEncodeElement::GetQueueDataTotalLen()
{
	uint32_t total_len = 0;
	for (auto& item : m_data_que) {
		total_len += item->media_data->data_len;
	}

	return total_len;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioEncodeElement::FillAudioData(uint8_t* data, uint32_t data_len, 
	PinData& encode_pin_data)
{
	uint32_t write_pos = 0;

	while (data_len > 0) {
		PinDataSP pin_data = m_data_que.front();
		auto para = SPC<media::AudioFramePara>(encode_pin_data.media_para);
		if (para->codec == media::AudioCodec::INVALID) {
			encode_pin_data.media_para = pin_data->media_para;
			para->codec = m_sink_pin_cap.codec;

			encode_pin_data.pts = pin_data->pts + pin_data->media_data[0].start_pos
				/ media::util::GetChnlsNum(m_sink_pin_cap.chnls)
				* 1000 / media::util::GetSRateNum(m_sink_pin_cap.srate);

			encode_pin_data.dts = encode_pin_data.pts;
			encode_pin_data.drt = OPUS_PACK_DURATION;
			encode_pin_data.tbn = 0;
			encode_pin_data.tbd = 0;
		}

		if (pin_data->media_data[0].data_len >= data_len) {
			memcpy(data + write_pos, DP(pin_data->media_data[0]), data_len);

			pin_data->media_data[0].start_pos += data_len;
			pin_data->media_data[0].data_len -= data_len;

			data_len = 0;
		}
		else {
			memcpy(data + write_pos, DP(pin_data->media_data[0]), 
				pin_data->media_data[0].data_len);

			data_len -= pin_data->media_data[0].data_len;
			write_pos += pin_data->media_data[0].data_len;

			m_data_que.pop_front();
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioEncodeElement::ThreadProc()
{
	while (!m_stop) {
		std::unique_lock<std::mutex> lock(m_mutex);
		while (m_data_que.empty() || GetQueueDataTotalLen() < m_input_len) {
			m_con_var.wait_for(lock, std::chrono::milliseconds(10)); // release mutex
			if (m_stop) break;
		}

		while (GetQueueDataTotalLen() >= m_input_len) {
			uint8_t* raw_audio_data = new uint8_t[m_input_len];
			PinData encoded_data(media::MediaType::AUDIO, m_input_len);

			FillAudioData(raw_audio_data, m_input_len, encoded_data);

			opus_int32 result = opus_encode(m_opus_encoder,
				(opus_int16*)raw_audio_data,
				media::util::GetSRateNum(m_sink_pin_cap.srate) * OPUS_PACK_DURATION / 1000,
				encoded_data.media_data->data.get(),
				encoded_data.media_data->total_len);
			if (result <= 0) {
				LOG_ERR("Opus encode failed, error:{}", result);
				delete[] raw_audio_data;
				continue;
			}
			encoded_data.media_data->data_len = result;
			SRC_PIN->OnPinData(encoded_data);

			if (m_ae_dumper) {
				m_ae_dumper->WriteData(DP(encoded_data.media_data[0]),
					encoded_data.media_data[0].data_len);
			}

			delete[] raw_audio_data;
		}
	}
}

}