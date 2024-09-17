#include "audio-mix-element.h"
#include "util-streamer.h"
#include "pipeline-msg.h"
#include "common/util-time.h"

#include <functional>


using namespace jukey::com;

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AudioMixElement::AudioMixElement(base::IComFactory* factory, const char* owner)
	: ProxyUnknown(nullptr)
  , ComObjTracer(factory, CID_AUDIO_MIX, owner)
	, ElementBase(factory)
	, util::CommonThread("audio-mix-element", true)
{
	m_ele_name   = CONSTRUCT_ELEMENT_NAME("a-mix-");
	m_main_type  = EleMainType::FILTER;
	m_sub_type   = EleSubType::MIXER;
	m_media_type = EleMediaType::AUDIO_ONLY;

	if (ERR_CODE_OK != CreateSrcPin()) {
		m_pins_created = false;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AudioMixElement::~AudioMixElement()
{
	LOG_INF("Destruct {}", m_ele_name);

	// TODO: release
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* AudioMixElement::CreateInstance(base::IComFactory* factory, 
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_AUDIO_MIX) == 0) {
		return new AudioMixElement(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* AudioMixElement::NDQueryInterface(const char* riid)
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
ErrCode AudioMixElement::CreateSrcPin()
{
	media::com::AudioCaps src_pin_caps;
	src_pin_caps.AddCap(media::AudioCodec::PCM);

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

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioMixElement::DoInit(com::IProperty* props)
{
	m_logger = g_logger;

	m_pipeline->SubscribeMsg((uint32_t)PlMsgType::ADD_SINK_PIN, this);
	m_pipeline->SubscribeMsg((uint32_t)PlMsgType::REMOVE_SINK_PIN, this);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// Nothing to do
//------------------------------------------------------------------------------
ErrCode AudioMixElement::DoStart()
{
	StartThread();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioMixElement::OnSinkPinData(ISinkPin* pin, const PinData& data)
{
	LOG_DBG("OnSinkPinData, len:{}", data.media_data[0].data_len);

	if (m_ele_state != EleState::RUNNING) {
		LOG_DBG("Element is not running!");
		return ERR_CODE_FAILED;
	}

	if (data.mt != media::MediaType::VIDEO) {
		LOG_DBG("Invalid media type:{}", data.mt);
		return ERR_CODE_FAILED;
	}

	// TODO: 理论上这个值不会变化
	if (m_av_sync_id != data.syn) {
		m_av_sync_id = data.syn;
		m_pipeline->GetSyncMgr().AddSyncHandler(m_av_sync_id, this);
	}

	PinDataSP new_data = media:: util::ClonePinData(data);
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_data_que.push(new_data);
	}
	m_con_var.notify_all();

	if (m_data_que.size() > 64) {
		//return ERR_CODE_SEND_PENDING;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioMixElement::OnSinkPinNegotiated(ISinkPin* pin, const std::string& cap)
{
	LOG_INF("Sink pin negotiated, cap:{}", media::util::Capper(cap));

	auto vcap = media::util::ParseVideoCap(cap);
	if (!vcap.has_value()) {
		LOG_ERR("Parse video cap failed");
		return ERR_CODE_FAILED;
	}

	m_sink_pin_cap = vcap.value();

	// TODO:

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioMixElement::OnSrcPinNegotiated(ISrcPin* pin, const std::string& cap)
{
	LOG_INF("Src pin negotiated, cap:{}", media::util::Capper(cap));

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioMixElement::OnAddSinkPin(const com::CommonMsg& msg)
{
	LOG_INF("OnAddSinkPin");

	media::com::AudioCaps sink_pin_caps;
	sink_pin_caps.AddCap(media::AudioCodec::PCM);

	stmr::ISinkPin* sink_pin = (ISinkPin*)QI(CID_SINK_PIN, IID_SINK_PIN, 
		m_ele_name);
	if (!sink_pin) {
		LOG_ERR("Create sink pin failed!");
		return ERR_CODE_FAILED;
	}

	if (com::ERR_CODE_OK != sink_pin->Init(
		media::MediaType::AUDIO,
		this,
		CONSTRUCT_PIN_NAME("audio-sink-pin-"),
		media::util::ToAudioCapsStr(sink_pin_caps),
		this)) {
		LOG_ERR("Init auido sink pin failed!");
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioMixElement::OnRemoveSinkPin(const com::CommonMsg& msg)
{
	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioMixElement::ThreadProc()
{
	while (!m_stop) {
		std::unique_lock<std::mutex> lock(m_mutex);
		while (m_data_que.empty()) {
			m_con_var.wait(lock); // release mutex
		}

		PinDataSP data = m_data_que.front();

		// 未开启同步或者视频时间戳落后于音频时间戳
		if (m_audio_pts == 0 || data->pts <= m_audio_pts) {

			m_data_que.pop();
		}
		else {
			lock.unlock();
			util::Sleep(10);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioMixElement::OnSyncUpdate(uint64_t timestamp)
{
	m_audio_pts = timestamp;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode AudioMixElement::PreProcPipelineMsg(const CommonMsg& msg)
{
	switch (msg.msg_type) {
	case (uint32_t)stmr::PlMsgType::ADD_SINK_PIN:
		return OnAddSinkPin(msg);
	case (uint32_t)stmr::PlMsgType::REMOVE_SINK_PIN:
		return OnRemoveSinkPin(msg);
	default:
		return ERR_CODE_MSG_NO_PROC;
	}
}

}