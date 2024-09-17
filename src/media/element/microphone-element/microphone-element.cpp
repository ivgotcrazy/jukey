#include "microphone-element.h"
#include "util-streamer.h"
#include "util-enum.h"
#include "common/util-time.h"
#include "common/util-common.h"
#include "pipeline-msg.h"
#include "log.h"
#include "common/media-common-define.h"
#include "yaml-cpp/yaml.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wmsdkidl.h>

#pragma comment(lib,"mfplat.lib")
#pragma comment(lib,"mf.lib")
#pragma comment(lib,"Mfreadwrite.lib")
#pragma comment(lib,"mfuuid.lib")
#pragma comment(lib,"evr.lib")
#pragma comment(lib,"shlwapi.lib")


using namespace jukey::com;
using namespace jukey::util;

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MicrophoneElement::MicrophoneElement(base::IComFactory* factory,
	const char* owner)
	: ProxyUnknown(nullptr)
  , ComObjTracer(factory, CID_MICROPHONE, owner)
	, ElementBase(factory)
	, CommonThread("microphone element", true)
{
	m_ele_name   = CONSTRUCT_ELEMENT_NAME("microphone-");
	m_main_type  = EleMainType::SRC;
	m_sub_type   = EleSubType::CAPTURER;
	m_media_type = EleMediaType::AUDIO_ONLY;

	if (ERR_CODE_OK != CreateSrcPin()) {
		m_pins_created = false;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MicrophoneElement::~MicrophoneElement()
{
	LOG_INF("Destruct {}", m_ele_name);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* MicrophoneElement::CreateInstance(base::IComFactory* factory, 
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_MICROPHONE) == 0) {
		return new MicrophoneElement(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* MicrophoneElement::NDQueryInterface(const char* riid)
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
ErrCode MicrophoneElement::CreateSrcPin()
{
	media::com::AudioCaps acaps;
	acaps.AddCap(media::AudioCodec::PCM);
	acaps.AddCap(media::AudioChnls::STEREO);
	acaps.AddCap(media::AudioChnls::MONO);
	acaps.AddCap(media::AudioSBits::S16);
	acaps.AddCap(media::AudioSBits::S16P);
	acaps.AddCap(media::AudioSBits::S32);
	acaps.AddCap(media::AudioSBits::S32P);
	acaps.AddCap(media::AudioSRate::SR_48K);
	acaps.AddCap(media::AudioSRate::SR_16K);

	ISrcPin* src_pin = (ISrcPin*)QI(CID_SRC_PIN, IID_SRC_PIN, m_ele_name);
	if (!src_pin) {
		LOG_ERR("Create src pin failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != src_pin->Init(media::MediaType::AUDIO,
			this,
			CONSTRUCT_PIN_NAME("a-src-pin-"),
			media::util::ToAudioCapsStr(acaps),
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
ErrCode MicrophoneElement::NotifyAudioStream()
{
	m_stream_id = util::GenerateGUID();

	if (com::ERR_CODE_OK != SRC_PIN->OnPinMsg(nullptr,
		PinMsg(PinMsgType::SET_STREAM, m_stream_id))) {
		LOG_ERR("Set src pin:{} stream failed!", SRC_PIN->Name());
		return ERR_CODE_FAILED;
	}

	ElementStream* es = new ElementStream();
	es->stream.src.src_type = MediaSrcType::MICROPHONE;
	es->stream.src.src_id = std::to_string(m_device_id);
	es->stream.stream.stream_type = StreamType::AUDIO;
	es->stream.stream.stream_id = m_stream_id;
	es->pin.ele_name = m_ele_name;
	es->pin.pin_name = m_src_pins[0]->Name();
	es->cap = m_src_pins[0]->Cap();

	com::CommonMsg msg;
	msg.msg_type = (uint32_t)PlMsgType::ADD_ELEMENT_STREAM;
	msg.src = m_ele_name;
	msg.msg_data.reset(es);
	m_pipeline->PostPlMsg(msg);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool MicrophoneElement::ParseProperties(com::IProperty* props)
{
	if (!props) {
		LOG_ERR("Invalid props!");
		return false;
	}

	uint32_t* device = props->GetU32Value("device-id");
	if (device) {
		m_device_id = *device;
		LOG_INF("Read device-id:{}", m_device_id);
	}
	else {
		LOG_ERR("Cannot find media-src property!");
		return false;
	}

	uint32_t* channels = props->GetU32Value("sample-chnl");
	if (channels) {
		m_sample_chnl = media::util::ToAudioChnls(*channels);
		m_src_pin_cap.chnls = m_sample_chnl; // TODO: 应该使用协商结果
		LOG_INF("Read channels:{}", media::util::AUDIO_CHNLS_STR(m_sample_chnl));
	}
	else {
		LOG_ERR("Cannot find sample-chnl property!");
		return false;
	}

	uint32_t* sample_rate = props->GetU32Value("sample-rate");
	if (sample_rate) {
		m_sample_rate = media::util::ToAudioSRate(*sample_rate);
		m_src_pin_cap.srate = m_sample_rate;
		LOG_INF("Read sample-rate:{}", media::util::AUDIO_SRATE_STR(m_sample_rate));
	}
	else {
		LOG_ERR("Cannot find sample-rate property!");
		return false;
	}

	uint32_t* sample_bits = props->GetU32Value("sample-bits");
	if (sample_bits) {
		m_sample_bits = media::util::ToAudioSBits(*sample_bits);
		m_src_pin_cap.sbits = m_sample_bits;
		LOG_INF("Read sample-bits:{}", media::util::AUDIO_SBITS_STR(m_sample_bits));
	}
	else {
		LOG_ERR("Cannot find sample-bits property!");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool MicrophoneElement::UpdateAvaiCaps()
{
	media::com::AudioCap acap;
	acap.codec = media::AudioCodec::PCM;
	acap.chnls = m_sample_chnl;
	acap.srate = m_sample_rate;
	acap.sbits = m_sample_bits;

	PinCaps avai_caps;
	avai_caps.push_back(media::util::ToAudioCapStr(acap));

	LOG_INF("Update available caps:");

	if (ERR_CODE_OK != SRC_PIN->UpdateAvaiCaps(avai_caps)) {
		LOG_ERR("Update src pin available caps failed");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MicrophoneElement::EnumMicrophoneDevice()
{
	IMFAttributes* pAttributes = nullptr;
	HRESULT hr = MFCreateAttributes(&pAttributes, 1);
	if (FAILED(hr)) {
		LOG_ERR("MFCreateAttributes failed!");
		return com::ERR_CODE_FAILED;
	}

	hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID);
	if (FAILED(hr)) {
		LOG_ERR("SetGUID failed!");
		return com::ERR_CODE_FAILED;
	}

	IMFActivate** ppDevices;
	UINT32 count = 0;

	hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
	if (FAILED(hr)) {
		LOG_ERR("MFEnumDeviceSources failed!");
		return com::ERR_CODE_FAILED;
	}

	if (m_device_id + 1 > count) {
		LOG_ERR("Unexpected device count:{}, device id:{}", count, m_device_id);
		return com::ERR_CODE_FAILED;
	}

	m_audio_device = ppDevices[m_device_id];

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MicrophoneElement::ParseElementConfig()
{
	try {
		YAML::Node root = YAML::LoadFile(ELEMENT_CONFIG_FILE);

		auto node = root["microphone-element"];
		if (!node) {
			return;
		}

		if (node["dump-microphone-data"]) {
			if (node["dump-microphone-data"].as<bool>()) {
				m_mic_dumper.reset(new util::DataDumper(m_ele_name + ".data"));
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
ErrCode MicrophoneElement::DoInit(com::IProperty* props)
{
	m_logger = g_logger;

	if (!props) {
		LOG_ERR("Invalid properties!");
		return ERR_CODE_INVALID_PARAM;
	}

	LOG_INF("DoInit, props:{}", props->Dump());

	if (!ParseProperties(props)) {
		LOG_ERR("Parse properties failed!");
		return ERR_CODE_INVALID_PARAM;
	}

	if (ERR_CODE_OK != EnumMicrophoneDevice()) {
		LOG_ERR("Enum microphone device failed!");
		return ERR_CODE_FAILED;
	}

	if (!UpdateAvaiCaps()) {
		LOG_ERR("Update available caps failed!");
		return ERR_CODE_FAILED;
	}

	HRESULT hr = S_OK;
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr)) {
		LOG_ERR("CoInitializeEx failed!");
		return ERR_CODE_FAILED;
	}
	
	hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
	if (FAILED(hr)) {
		LOG_ERR("MFStartup failed!");
		return ERR_CODE_FAILED;
	}

	if (com::ERR_CODE_OK != CreateMediaSource()) {
		LOG_ERR("Create media source failed!");
		return ERR_CODE_FAILED;
	}

	if (com::ERR_CODE_OK != NotifyAudioStream()) {
		LOG_ERR("Notify audio stream failed!");
		return ERR_CODE_FAILED;
	}

	ParseElementConfig();

	m_data_stats.reset(new DataStats(m_factory, g_logger, m_ele_name));
	m_data_stats->Start();

	StatsParam fr_stats("framerate", StatsType::IAVER, 1000);
	m_fr_stats = m_data_stats->AddStats(fr_stats);

	StatsParam br_stats("bitrate", StatsType::IAVER, 1000);
	m_br_stats = m_data_stats->AddStats(br_stats);

	m_pipeline->SubscribeMsg(PlMsgType::NEGOTIATE, this);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MicrophoneElement::DoStart()
{
	LOG_INF("DoStart");

	StartThread();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MicrophoneElement::CreateSourceReader()
{
	HRESULT hr = MFCreateSourceReaderFromMediaSource(m_media_source,
		NULL, &m_source_reader);
	if (FAILED(hr)) {
		LOG_ERR("MFCreateSourceReaderFromMediaSource failed!");
		return ERR_CODE_FAILED;
	}

	IMFMediaType* pMediaType = nullptr;
	hr = MFCreateMediaType(&pMediaType);
	if (FAILED(hr)) {
		LOG_ERR("MFCreateMediaType failed!");
		return ERR_CODE_FAILED;
	}

	pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	pMediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);

	hr = pMediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 
		media::util::GetChnlsNum(m_sample_chnl));
	if (FAILED(hr)) {
		LOG_ERR("Set auido channels:{} failed!", m_sample_chnl);
		return ERR_CODE_FAILED;
	}

	hr = pMediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 
		media::util::GetSRateNum(m_sample_rate));
	if (FAILED(hr)) {
		LOG_ERR("Set auido sample rate:{} failed!", m_sample_rate);
		return ERR_CODE_FAILED;
	}

	hr = pMediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 
		media::util::GetSBitsNum(m_sample_bits));
	if (FAILED(hr)) {
		LOG_ERR("Set auido sample bits:{} failed!", m_sample_bits);
		return ERR_CODE_FAILED;
	}

	hr = pMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1);
	if (FAILED(hr)) {
		LOG_ERR("Set auido samples indepent failed!");
		return ERR_CODE_FAILED;
	}

	hr = m_source_reader->SetCurrentMediaType(
		MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, pMediaType);
	if (FAILED(hr)) {
		LOG_ERR("SetCurrentMediaType failed, hr:{}, err:{}", hr, GetLastError());
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MicrophoneElement::CreateMediaSource()
{
	HRESULT hr = m_audio_device->ActivateObject(IID_PPV_ARGS(&m_media_source));
	if (FAILED(hr)) {
		LOG_ERR("MFEnumDeviceSources failed!");
		return ERR_CODE_FAILED;
	}
	m_media_source->AddRef();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MicrophoneElement::ProcSampleSync()
{
	DWORD stream_index, flags;
	LONGLONG timestamp;
	IMFSample* sample = NULL;

	static uint64_t time_start = util::Now();

	HRESULT hr = m_source_reader->ReadSample(
		MF_SOURCE_READER_ANY_STREAM, // Stream index.
		0,                           // Flags.
		&stream_index,               // Receives the actual stream index. 
		&flags,                      // Receives status flags.
		&timestamp,                  // Receives the time stamp.
		&sample                      // Receives the sample or NULL.
	);
	if (FAILED(hr)) {
		LOG_ERR("Read sample failed, hr:{}, error:{}", hr, GetLastError());
		return;
	}

	if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
		LOG_WRN("\tEnd of stream\n");
		m_stop = true;
	}

	if (flags & MF_SOURCE_READERF_NEWSTREAM) {
		LOG_INF("\tNew stream\n");
	}

	if (flags & MF_SOURCE_READERF_NATIVEMEDIATYPECHANGED) {
		LOG_INF("\tNative type changed\n");
	}

	if (flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) {
		LOG_INF("\tCurrent type changed\n");
	}

	if (flags & MF_SOURCE_READERF_STREAMTICK) {
		LOG_INF("\tStream tick\n");
	}

	if (sample) {
		IMFMediaBuffer* media_buf = nullptr;
		sample->GetBufferByIndex(0, &media_buf);
		if (media_buf) {
			BYTE* buf;
			DWORD max_len, cur_len;
			if (SUCCEEDED(media_buf->Lock(&buf, &max_len, &cur_len))) {
				// Calculate sample count
				uint32_t sample_count = cur_len
					/ (media::util::GetSBitsNum(m_src_pin_cap.sbits) / 8)
					/ (media::util::GetChnlsNum(m_src_pin_cap.chnls));

				m_data_stats->OnData(m_fr_stats, sample_count);
				m_data_stats->OnData(m_br_stats, cur_len);

				// Copy sample data
				PinData pin_data(media::MediaType::AUDIO, buf, cur_len);

				// Set data attributes
				pin_data.dts = util::Now() - time_start;
				pin_data.pts = pin_data.dts;
				pin_data.drt = 0;
				pin_data.pos = 0;
				pin_data.syn = 0;
				pin_data.tbn = 1;
				pin_data.tbd = 1000;
				pin_data.data_count = 1;

				auto para = SPC<media::AudioFramePara>(pin_data.media_para);

				// Set audo frame parameters
				para->codec = m_src_pin_cap.codec;
				para->chnls = m_src_pin_cap.chnls;
				para->power = 0;
				para->count = sample_count;
				para->ts = (uint32_t)(util::Now() - time_start);
				para->srate = m_src_pin_cap.srate;
				para->seq = m_frame_seq;

				// Dump audio frame data
				if (m_mic_dumper) {
					m_mic_dumper->WriteData(DP(pin_data.media_data[0]),
						pin_data.media_data[0].data_len);
				}

				// Flow down
				if (m_ele_state == EleState::RUNNING) {
					SRC_PIN->OnPinData(pin_data);
				}
				
				m_frame_seq += sample_count;
			}

			media_buf->Unlock();
			media_buf->Release();
		}
		else {
			LOG_ERR("Lock buffer failed!");
		}
		sample->Release();
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MicrophoneElement::ProcFrameRateStats()
{
	static uint64_t frame_count = 0;
	static uint64_t last_stats_time = util::Now();

	++frame_count;

	uint64_t now = util::Now();

	if (now - last_stats_time >= 1000) {
		LOG_INF("Capture frame count:{}", frame_count);
		last_stats_time = now;
		frame_count = 0;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MicrophoneElement::ThreadProc()
{
	while (!m_stop) {
		if (m_ele_state != EleState::RUNNING || !m_source_reader) {
			util::Sleep(10);
			continue;
		}
		ProcSampleSync();
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MicrophoneElement::OnPipelineNegotiateMsg(const com::CommonMsg& msg)
{
	if (ERR_CODE_OK != SRC_PIN->Negotiate()) {
		LOG_ERR("Negotiate failed!");
		if (msg.result) {
			msg.result->set_value(ERR_CODE_FAILED);
		}
	}
	else {
		LOG_INF("Negotiate success");
		if (msg.result) {
			msg.result->set_value(ERR_CODE_OK);
		}
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode MicrophoneElement::PreProcPipelineMsg(const CommonMsg& msg)
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
ErrCode 
MicrophoneElement::OnSrcPinNegotiated(ISrcPin* pin, const std::string& cap)
{
	LOG_INF("Src pin negotiated, cap:{}", media::util::Capper(cap));

	auto src_cap = media::util::ParseAudioCap(cap);
	if (!src_cap.has_value()) {
		LOG_ERR("Parse audio cap failed");
		return ERR_CODE_FAILED;
	}

	m_src_pin_cap = src_cap.value();

	return CreateSourceReader();
}

}