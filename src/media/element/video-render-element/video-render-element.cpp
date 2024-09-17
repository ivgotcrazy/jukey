#include <functional>

#include "video-render-element.h"
#include "util-sdl.h"
#include "util-streamer.h"
#include "pipeline-msg.h"
#include "common/util-time.h"
#include "log.h"
#include "common/media-common-define.h"
#include "yaml-cpp/yaml.h"

#define MAX_BUF_FRAME 8
#define MIN_BUF_FRAME 2

using namespace jukey::com;
using namespace jukey::media::util;

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
VideoRenderElement::VideoRenderElement(base::IComFactory* factory,
	const char* owner)
	: ProxyUnknown(nullptr)
	, ComObjTracer(factory, CID_VIDEO_RENDER, owner)
	, ElementBase(factory)
	, util::CommonThread("video-render-element", true)
{
	m_ele_name   = CONSTRUCT_ELEMENT_NAME("v-render-");
	m_main_type  = EleMainType::SINK;
	m_sub_type   = EleSubType::PLAYER;
	m_media_type = EleMediaType::VIDEO_ONLY;

	if (ERR_CODE_OK != CreateSinkPin()) {
		m_pins_created = false;
	}

	m_sdl_renderer = std::make_shared<SdlRenderer>();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
VideoRenderElement::~VideoRenderElement()
{
	LOG_INF("Destruct {}", m_ele_name);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* VideoRenderElement::CreateInstance(base::IComFactory* factory, 
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_VIDEO_RENDER) == 0) {
		return new VideoRenderElement(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* VideoRenderElement::NDQueryInterface(const char* riid)
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
ErrCode VideoRenderElement::CreateSinkPin()
{
	media::com::VideoCaps sink_pin_caps;
	sink_pin_caps.AddCap(media::VideoCodec::RAW);
	sink_pin_caps.AddCap(media::PixelFormat::I420);
	sink_pin_caps.AddCap(media::VideoRes::RES_1920x1080);
	sink_pin_caps.AddCap(media::VideoRes::RES_1280x720);
	sink_pin_caps.AddCap(media::VideoRes::RES_640x480);
	sink_pin_caps.AddCap(media::VideoRes::RES_640x360);
	sink_pin_caps.AddCap(media::VideoRes::RES_320x240);
	sink_pin_caps.AddCap(media::VideoRes::RES_320x180);

	ISinkPin* sink_pin = (ISinkPin*)QI(CID_SINK_PIN, IID_SINK_PIN, m_ele_name);
	if (!sink_pin) {
		LOG_ERR("Create sink pin failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != sink_pin->Init(media::MediaType::VIDEO,
			this,
			CONSTRUCT_PIN_NAME("v-sink-pin-"),
			media::util::ToVideoCapsStr(sink_pin_caps),
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
void VideoRenderElement::ParseElementConfig()
{
	try {
		YAML::Node root = YAML::LoadFile(ELEMENT_CONFIG_FILE);

		auto node = root["video-render-element"];
		if (!node) {
			return;
		}

		if (node["dump-render-data"]) {
			if (node["dump-render-data"].as<bool>()) {
				m_stream_dumper.reset(new StreamDumper(m_ele_name + ".data"));
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
ErrCode VideoRenderElement::DoInit(com::IProperty* props)
{
	m_logger = g_logger;

	if (!props) {
		LOG_ERR("Invalid properties!");
		return ERR_CODE_INVALID_PARAM;
	}

	LOG_INF("DoInit, props:{}", props->Dump());

	m_render_wnd = (void*)props->GetPtrValue("render-wnd");
	//if (!m_render_wnd) {
	//	LOG_ERR("Cannot find 'render-wnd' in properties!");
	//	return ERR_CODE_INVALID_PARAM;
	//}

	com::MainThreadExecutor* executor = 
		(com::MainThreadExecutor*)props->GetPtrValue("executor");
	if (!executor) {
		LOG_ERR("Cannot find 'executor' in properties!");
		return ERR_CODE_INVALID_PARAM;
	}

	if (!m_sdl_renderer->Init(executor, m_render_wnd)) {
		LOG_ERR("Init sdl renderer failed!");
		return ERR_CODE_FAILED;
	}

	m_data_stats.reset(new util::DataStats(m_factory, g_logger, m_ele_name));
	m_data_stats->Start();

	util::StatsParam fr_stats("frame-rate", util::StatsType::IAVER, 1000);
	m_fr_stats = m_data_stats->AddStats(fr_stats);

	m_pipeline->SubscribeMsg((uint32_t)PlMsgType::REMOVE_RENDERER, this);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoRenderElement::DoStart()
{
	return m_sdl_renderer->Start();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoRenderElement::DoPause()
{
	return m_sdl_renderer->Pause();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoRenderElement::DoResume()
{
	return m_sdl_renderer->Resume();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoRenderElement::DoStop()
{
	return m_sdl_renderer->Stop();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode 
VideoRenderElement::ProcSinkPinMsg(stmr::ISinkPin* pin, const stmr::PinMsg& msg)
{
	if (msg.msg_type == PinMsgType::FLUSH_DATA) {
		m_sdl_renderer->FlushData();
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoRenderElement::OnSinkPinData(ISinkPin* pin, const PinData& data)
{
	LOG_DBG("OnSinkPinData, len:{}", data.media_data[0].data_len);

	if (m_ele_state != EleState::RUNNING) {
		LOG_DBG("Element is not running!");
		return ERR_CODE_FAILED;
	}

	if (m_stream_dumper) {
		m_stream_dumper->WriteStreamData(data);
	}
	
	m_data_stats->OnData(m_fr_stats, 1);

	// TODO: 理论上这个值不会变化
	if (m_av_sync_id != data.syn) {
		m_av_sync_id = data.syn;
		m_pipeline->GetSyncMgr().AddSyncHandler(m_av_sync_id, this);
	}

	m_sdl_renderer->OnVideoFrame(data);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode 
VideoRenderElement::OnSinkPinNegotiated(ISinkPin* pin, const std::string& cap)
{
	LOG_INF("Sink pin negotiated, cap:{}", media::util::Capper(cap));

	auto sink_cap = media::util::ParseVideoCap(cap);
	if (!sink_cap.has_value()) {
		LOG_ERR("Parse video cap failed!");
		return ERR_CODE_FAILED;
	}

	m_sdl_renderer->OnSinkPinNegotiated(sink_cap.value());

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// All return OK???
//------------------------------------------------------------------------------
ErrCode VideoRenderElement::OnRemoveRenderer(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(stmr::RemoveRendererData);

	if (data->render_wnd != m_render_wnd) {
		LOG_ERR("Unknown render window!");
		return ERR_CODE_FAILED; // does not belong here
	}

	ISrcPin* src_pin = SINK_PIN->SrcPin(); // TODO:
	if (!src_pin) {
		LOG_ERR("Invalid src pin!");
		msg.result->set_value(ERR_CODE_FAILED);
		return ERR_CODE_OK;
	}

	// Remove sink pin from src pin
	if (ERR_CODE_OK != src_pin->RemoveSinkPin(
		SINK_PIN->Name())) {
		LOG_ERR("Remove sink pin:{} failed!", SINK_PIN->Name());
		msg.result->set_value(ERR_CODE_FAILED);
		return ERR_CODE_OK;
	}

	// Reply to remove renderer synchronously
	msg.result->set_value(ERR_CODE_OK);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoRenderElement::OnSyncUpdate(uint64_t timestamp)
{
	m_sdl_renderer->UpdateAudioTimestamp(timestamp);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoRenderElement::PreProcPipelineMsg(const CommonMsg& msg)
{
	switch (msg.msg_type) {
	case PlMsgType::REMOVE_RENDERER:
		return OnRemoveRenderer(msg);
	default:
		return ERR_CODE_MSG_NO_PROC;
	}
}

}