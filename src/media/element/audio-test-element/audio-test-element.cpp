#include "audio-test-element.h"
#include "util-streamer.h"
#include "util-ffmpeg.h"
#include "util-streamer.h"
#include "common/util-common.h"
#include "pipeline-msg.h"
#include "log.h"
#include "common/media-common-define.h"

using namespace jukey::com;

namespace
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
#define VOLUMEMAX   32767
int CalcAudioDB(short* data, int sample)
{
	signed short ret = 0;
	int sum = 0;
	signed short* pos = (signed short*)data;

	if (sample <= 0) return 0;

	for (int i = 0; i < sample; i++) {
		sum += abs(*pos);
		pos++;
	}

	ret = static_cast<short>(sum * 500.0 / (sample * VOLUMEMAX));
	if (ret >= 100) {
		ret = 100;
	}

	return ret;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
int CalcAudioEnergy(const int16_t* data, size_t length)
{
	static const float kMaxSquaredLevel = 32768 * 32768;
	constexpr float kMinLevel = 30.f;

	float sum_square_ = 0;
	for (size_t i = 0; i < length; ++i) {
		sum_square_ += data[i] * data[i];
	}

	float rms = 10 * log10(sum_square_ / (length * kMaxSquaredLevel));

	if (rms < -kMinLevel)
		rms = -kMinLevel;

	rms = -rms;

	return static_cast<int>(kMinLevel - rms);
}

}

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AudioTestElement::AudioTestElement(base::IComFactory* factory, const char* owner)
	: base::ProxyUnknown(nullptr)
  , base::ComObjTracer(factory, CID_AUDIO_TEST, owner)
	, ElementBase(factory)
{
	m_ele_name   = CONSTRUCT_ELEMENT_NAME("a-test-");
	m_main_type  = EleMainType::SINK;
	m_sub_type   = EleSubType::TESTER;
	m_media_type = EleMediaType::AUDIO_ONLY;

	if (ERR_CODE_OK != CreateSinkPin()) {
		m_pins_created = false;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AudioTestElement::~AudioTestElement()
{
	LOG_INF("Destruct {}", m_ele_name);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* AudioTestElement::CreateInstance(base::IComFactory* factory, 
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_AUDIO_TEST) == 0) {
		return new AudioTestElement(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* AudioTestElement::NDQueryInterface(const char* riid)
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
ErrCode AudioTestElement::CreateSinkPin()
{
	media::com::AudioCaps audio_pin_caps;
	audio_pin_caps.AddCap(media::AudioCodec::PCM);
	audio_pin_caps.AddCap(media::AudioChnls::STEREO);
	audio_pin_caps.AddCap(media::AudioChnls::MONO);
	audio_pin_caps.AddCap(media::AudioSBits::S16);
	audio_pin_caps.AddCap(media::AudioSBits::FLTP);
	audio_pin_caps.AddCap(media::AudioSRate::SR_48K);
	audio_pin_caps.AddCap(media::AudioSRate::SR_16K);

	ISinkPin* sink_pin = (ISinkPin*)QI(CID_SINK_PIN, IID_SINK_PIN, m_ele_name);
	if (!sink_pin) {
		LOG_ERR("Create sink pin failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != sink_pin->Init(media::MediaType::AUDIO,
			this,
			CONSTRUCT_PIN_NAME("audio-sink-pin-"),
			media::util::ToAudioCapsStr(audio_pin_caps),
			this)) {
		LOG_ERR("Init audio sink pin failed!");
		return ERR_CODE_FAILED;
	}

	m_sink_pin_index = static_cast<uint32_t>(m_sink_pins.size());
	m_sink_pins.push_back(sink_pin);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioTestElement::OnTimeout()
{
	CommonMsg msg(PlMsgType::AUDIO_ENERGY);
	msg.msg_data.reset(new AudioEnergyData(SINK_PIN->StreamId(), m_audio_energy));

	m_pipeline->PostPlMsg(msg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioTestElement::DoInit(com::IProperty* props)
{
	m_logger = g_logger;

	com::ITimerMgr* timer_mgr = QUERY_TIMER_MGR(m_factory);
	assert(timer_mgr);

	com::TimerParam timer_param;
	timer_param.timer_type = com::TimerType::TIMER_TYPE_LOOP;
	timer_param.timeout    = 100;
	timer_param.timer_name = "audio test element";
	timer_param.timer_func = [this](int64_t) {
		this->OnTimeout();
	};

	m_timer_id = timer_mgr->AllocTimer(timer_param);
	timer_mgr->StartTimer(m_timer_id);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioTestElement::OnSinkPinData(ISinkPin* pin, const PinData& data)
{
	if (m_ele_state != EleState::RUNNING) {
		LOG_ERR("Element is not running!");
		return ERR_CODE_FAILED;
	}

	// Calculate audio energy
	m_audio_energy = CalcAudioEnergy((int16_t*)data.media_data->data.get(), 
		data.media_data->data_len / 2);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode 
AudioTestElement::OnSinkPinNegotiated(ISinkPin* pin, const std::string& cap)
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

}