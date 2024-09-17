#include "audio-play-element.h"
#include "util-sdl.h"
#include "util-streamer.h"
#include "pipeline-msg.h"
#include "if-sync-mgr.h"
#include "common/util-common.h"
#include "common/media-common-define.h"
#include "log.h"
#include "yaml-cpp/yaml.h"


#define SYNC_CACHE_AUDIO_SIZE 10
#define ASYNC_CACHE_AUDIO_SIZE 16

using namespace jukey::util;
using namespace jukey::com;
using namespace jukey::media::util;

namespace
{

using namespace jukey::stmr;
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioSampleCallbak(void* user_data, uint8_t* data, int len)
{
	AudioPlayElement* element = (AudioPlayElement*)user_data;
	element->OnAudioSample(data, len);
}

}

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// Construction
//------------------------------------------------------------------------------
AudioPlayElement::AudioPlayElement(base::IComFactory* factory, const char* owner)
	: ProxyUnknown(nullptr)
  , ComObjTracer(factory, CID_AUDIO_PLAY, owner)
	, ElementBase(factory)
{
	m_ele_name   = CONSTRUCT_ELEMENT_NAME("a-play-");
	m_main_type  = EleMainType::SINK;
	m_sub_type   = EleSubType::PLAYER;
	m_media_type = EleMediaType::AUDIO_ONLY;

	if (ERR_CODE_OK != CreateSinkPin()) {
		m_pins_created = false;
	}

	m_fill_lbuf = new uint8_t[1024 * 16];
	m_fill_rbuf = new uint8_t[1024 * 16];
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AudioPlayElement::~AudioPlayElement()
{
	LOG_INF("Destruct {}", m_ele_name);

	SDL_CloseAudio();

	std::lock_guard<std::mutex> lock(m_mutex);

	delete[] m_fill_lbuf;
	m_fill_lbuf = nullptr;

	delete[] m_fill_rbuf;
	m_fill_rbuf = nullptr;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* AudioPlayElement::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_AUDIO_PLAY) == 0) {
		return new AudioPlayElement(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* AudioPlayElement::NDQueryInterface(const char* riid)
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
ErrCode AudioPlayElement::CreateSinkPin()
{
	media::com::AudioCaps sink_pin_caps;
	sink_pin_caps.AddCap(media::AudioCodec::PCM);
	sink_pin_caps.AddCap(media::AudioChnls::STEREO);
	sink_pin_caps.AddCap(media::AudioChnls::MONO);
	sink_pin_caps.AddCap(media::AudioSBits::S16);
	sink_pin_caps.AddCap(media::AudioSBits::S16P);
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
void AudioPlayElement::ParseElementConfig()
{
	try {
		YAML::Node root = YAML::LoadFile(ELEMENT_CONFIG_FILE);

		auto node = root["audio-play-element"];
		if (!node) {
			return;
		}

		if (node["dump-play-data"]) {
			if (node["dump-play-data"].as<bool>()) {
				m_data_dumper.reset(new StreamDumper(m_ele_name + "-bp.data"));
			}
		}

		// m_play_dumper.reset(new StreamDumper(m_ele_name + "-play.data"));
	}
	catch (const std::exception& e) {
		LOG_WRN("Error:{}", e.what());
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool AudioPlayElement::ParseProperties(com::IProperty* props)
{
	if (!props) {
		LOG_ERR("Invalid property!");
		return false;
	}

	uint32_t* value = props->GetU32Value("mode");
	if (value) {
		m_sync_mode = (*value != 0);
		LOG_INF("Read mode:{}", m_sync_mode);
	}
	else {
		LOG_ERR("Cannot find 'mode' property!");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioPlayElement::DoInit(com::IProperty* props)
{
	m_logger = g_logger;

	m_data_stats.reset(new DataStats(m_factory, m_logger, m_ele_name));

	StatsParam recv_fr_stats("recv-fr", StatsType::IAVER, 1000);
	m_recv_fr_stats = m_data_stats->AddStats(recv_fr_stats);

	StatsParam recv_br_stats("recv-br", StatsType::IAVER, 1000);
	m_recv_br_stats = m_data_stats->AddStats(recv_br_stats);

	StatsParam play_br_stats("play-br", StatsType::IAVER, 1000);
	m_play_br_stats = m_data_stats->AddStats(play_br_stats);

	StatsParam que_len_stats("que-len", StatsType::ICAVG, 1000);
	m_que_len_stats = m_data_stats->AddStats(que_len_stats);

	StatsParam drop_num_stats("drop", StatsType::IACCU, 1000);
	m_drop_num_stats = m_data_stats->AddStats(drop_num_stats);

	StatsParam miss_num_stats("miss", StatsType::IACCU, 1000);
	m_miss_num_stats = m_data_stats->AddStats(miss_num_stats);

	ParseElementConfig();

	if (!ParseProperties(props)) {
		LOG_ERR("Parse properties failed!");
		return ERR_CODE_INVALID_PARAM;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioPlayElement::OnAudioSample(uint8_t* data, int len)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_ele_state != EleState::RUNNING) {
		return;
	}

	if (!media::util::IsPlanar(m_sink_pin_cap.sbits)) {
		OnPackedAudioSample(data, len);
	}
	else if (m_sink_pin_cap.chnls == media::AudioChnls::STEREO) {
		OnPlanarStereoAudioSample(data, len);
	}
	else {
		LOG_ERR("Invalid audio channles:{}", m_sink_pin_cap.chnls);
	}

	if (m_sync_mode) {
		if (m_data_que.size() < SYNC_CACHE_AUDIO_SIZE) {
			m_con_var.notify_all();
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioPlayElement::OnPackedAudioSample(uint8_t* data, int len)
{
	while (len > 0) {
		// Current read done, then try to get a new audio frame
		if (m_fill_data_pos >= m_fill_data_len) {
			if (m_data_que.empty()) { // data queue is empty, fill with silence data
				m_fill_data_len = len;
				memset(m_fill_lbuf, 0, m_fill_data_len);
				m_data_stats->OnData(m_miss_num_stats, 1);
			}
			else { // data queue has data, read from data queue
				const PinDataSP& pin_data = m_data_que.front();

				memcpy(m_fill_lbuf, DP(pin_data->media_data[0]),
					pin_data->media_data[0].data_len);
				m_fill_data_len = pin_data->media_data[0].data_len;

				// m_play_dumper->WriteStreamData(*pin_data.get());

				// synchronize
				if (pin_data->tbn == 0) {
					m_pipeline->GetSyncMgr().UpdateTimestamp(pin_data->syn, 
						pin_data->pts);
				}
				else {
					m_pipeline->GetSyncMgr().UpdateTimestamp(pin_data->syn,
						pin_data->pts * pin_data->tbn * 1000 / pin_data->tbd);
				}

				m_data_que.pop();
			}
			m_fill_data_pos = 0;
		}

		// Calculate copy data size
		int32_t data_len = m_fill_data_len - m_fill_data_pos;
		if (data_len > len) {
			data_len = len;
		}

		// Copy data
		memcpy(data, m_fill_lbuf + m_fill_data_pos, data_len);

		m_data_stats->OnData(m_play_br_stats, data_len);

		len  -= data_len;
		data += data_len;

		m_fill_data_pos += data_len;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioPlayElement::OnPlanarStereoAudioSample(uint8_t* data, int len)
{
	while (len > 0) {
		if (m_fill_data_pos >= m_fill_data_len) {
			if (m_data_que.empty()) {
				m_fill_data_len = len;
				memset(m_fill_lbuf, 0, m_fill_data_len);
				memset(m_fill_rbuf, 0, m_fill_data_len);
				m_data_stats->OnData(m_miss_num_stats, 1);
			}
			else {
				const PinDataSP& pin_data = m_data_que.front();

				memcpy(m_fill_lbuf, DP(pin_data->media_data[0]),
					pin_data->media_data[0].data_len);
				memcpy(m_fill_rbuf, DP(pin_data->media_data[1]),
					pin_data->media_data[1].data_len);

				// m_play_dumper->WriteStreamData(*pin_data.get());

				// The two channel should have the same data size
				m_fill_data_len = pin_data->media_data[0].data_len;

				// Synchronize
				m_pipeline->GetSyncMgr().UpdateTimestamp(pin_data->syn, 
					pin_data->pts);

				m_data_que.pop();
			}
			m_fill_data_pos = 0;
		}

		// Calculate copy data size
		int32_t data_len = m_fill_data_len - m_fill_data_pos;
		if (data_len * 2 > len) {
			data_len = len / 2;
		}

		// Copy data
		memcpy(data, m_fill_lbuf + m_fill_data_pos, data_len);
		memcpy(data + data_len, m_fill_rbuf + m_fill_data_pos, data_len);

		len  -= data_len * 2;
		data += data_len * 2;

		m_fill_data_pos += data_len;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioPlayElement::OpenAudio()
{
	if (m_open_flag) return ErrCode::ERR_CODE_OK;

	if (!SINK_PIN->Negotiated()) {
		LOG_INF("Sink pin not negoited, cannot open audio!");
		return ERR_CODE_OK;
	}

	SDL_AudioSpec want, have;
	want.freq     = GetSRateNum(m_sink_pin_cap.srate);
	want.format   = ToSdlSampleFormat(m_sink_pin_cap.sbits);
	want.channels = GetChnlsNum(m_sink_pin_cap.chnls);
	want.samples  = 1024; // TODO:
	want.callback = AudioSampleCallbak;
	want.userdata = this;

	if (SDL_OpenAudio(&want, &have) != 0) {
		LOG_ERR("Open audio failed, error:{}", SDL_GetError());
		return ERR_CODE_FAILED;
	}

	LOG_INF("Open audio spec, sample rate:{}, sample format:{}, channels:{}",
		have.freq, have.format, have.channels);

	SDL_PauseAudio(0);

	m_open_flag = true;

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioPlayElement::CloseAudio()
{
	LOG_INF("CloseAudio");

	SDL_CloseAudio();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioPlayElement::DoStart()
{
	LOG_INF("DoStart");

	if (ERR_CODE_OK != OpenAudio()) {
		LOG_ERR("OpenAudio failed!");
		return ERR_CODE_FAILED;
	}

	m_data_stats->Start();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioPlayElement::DoPause()
{
	LOG_INF("DoPause");

	CloseAudio();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioPlayElement::DoResume()
{
	LOG_INF("DoResume");

	if (ERR_CODE_OK != OpenAudio()) {
		LOG_ERR("OpenAudio failed!");
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioPlayElement::DoStop()
{
	LOG_INF("DoStop");

	CloseAudio();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioPlayElement::OnSinkPinData(ISinkPin* pin, const PinData& data)
{
	LOG_DBG("Received sink pin data, len:{}", data.media_data[0].data_len);

	if (m_ele_state != EleState::RUNNING) {
		LOG_DBG("Invalid element state:{}!", m_ele_state);
		return ERR_CODE_FAILED;
	}

	assert(data.mt == media::MediaType::AUDIO);
	
	auto pin_data = ClonePinData(data);

	if (m_data_dumper) {
		m_data_dumper->WriteStreamData(*pin_data);
	}

	if (m_sync_mode) {
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_data_que.size() >= SYNC_CACHE_AUDIO_SIZE) {
			m_con_var.wait(m_mutex);
		}
		m_data_que.push(pin_data);
	}
	else {
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_data_que.size() >= ASYNC_CACHE_AUDIO_SIZE) {
			m_data_que.pop();
			m_data_stats->OnData(m_drop_num_stats, 1);
		}
		m_data_que.push(pin_data);
	}

	auto para = SPC<media::AudioFramePara>(data.media_para);
	m_data_stats->OnData(m_recv_fr_stats, para->count);
	m_data_stats->OnData(m_recv_br_stats, data.media_data->data_len);
	m_data_stats->OnData(m_que_len_stats, m_data_que.size());

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode
AudioPlayElement::OnSinkPinNegotiated(ISinkPin* pin, const std::string& cap)
{
	LOG_INF("Sink pin negotiated, cap:{}", Capper(pin->Cap()));

	auto sink_cap = media::util::ParseAudioCap(pin->Cap());
	if (!sink_cap.has_value()) {
		LOG_ERR("Parse audio cap failed");
		return ERR_CODE_FAILED;
	}

	m_sink_pin_cap = sink_cap.value();
	OpenAudio();

	return ERR_CODE_OK;
}

}