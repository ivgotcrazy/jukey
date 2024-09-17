#include "video-mix-element.h"
#include "log.h"
#include "util-streamer.h"
#include "common/media-common-define.h"

using namespace jukey::com;

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
VideoMixElement::VideoMixElement(base::IComFactory* factory, const char* owner)
	: base::ProxyUnknown(nullptr)
  , base::ComObjTracer(factory, CID_VIDEO_MIX, owner)
	, ElementBase(factory)
{
	m_ele_name   = CONSTRUCT_ELEMENT_NAME("v-mix-");
	m_main_type  = EleMainType::SRC;
	m_sub_type   = EleSubType::PROXY;
	m_media_type = EleMediaType::VIDEO_ONLY;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
VideoMixElement::~VideoMixElement()
{
	LOG_INF("Destruct {}", m_ele_name);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* VideoMixElement::CreateInstance(base::IComFactory* factory, 
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_VIDEO_MIX) == 0) {
		return new VideoMixElement(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* VideoMixElement::NDQueryInterface(const char* riid)
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
ErrCode VideoMixElement::CreateSrcPin()
{
	media::com::VideoCaps video_pin_caps;
	video_pin_caps.AddCap(media::VideoCodec::RAW);
	video_pin_caps.AddCap(media::PixelFormat::I420);
	video_pin_caps.AddCap(media::PixelFormat::NV12);
	video_pin_caps.AddCap(media::PixelFormat::YUY2);
	video_pin_caps.AddCap(media::PixelFormat::RGB24);
	video_pin_caps.AddCap(media::VideoRes::RES_1920x1080);
	video_pin_caps.AddCap(media::VideoRes::RES_1280x720);
	video_pin_caps.AddCap(media::VideoRes::RES_640x480);
	video_pin_caps.AddCap(media::VideoRes::RES_640x360);
	video_pin_caps.AddCap(media::VideoRes::RES_320x240);
	video_pin_caps.AddCap(media::VideoRes::RES_320x180);

	ISrcPin* src_pin = (ISrcPin*)QI(CID_SRC_PIN, IID_SRC_PIN, m_ele_name);
	if (!src_pin) {
		LOG_ERR("Create src pin failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != src_pin->Init(media::MediaType::VIDEO,
			this,
			CONSTRUCT_PIN_NAME("v-src-pin-"),
			media::util::ToVideoCapsStr(video_pin_caps),
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
ErrCode VideoMixElement::CreateSinkPin()
{
	media::com::VideoCaps video_pin_caps;
	video_pin_caps.AddCap(media::VideoCodec::RAW);
	video_pin_caps.AddCap(media::PixelFormat::I420);
	video_pin_caps.AddCap(media::PixelFormat::NV12);
	video_pin_caps.AddCap(media::PixelFormat::YUY2);
	video_pin_caps.AddCap(media::PixelFormat::RGB24);
	video_pin_caps.AddCap(media::VideoRes::RES_1920x1080);
	video_pin_caps.AddCap(media::VideoRes::RES_1280x720);
	video_pin_caps.AddCap(media::VideoRes::RES_640x480);
	video_pin_caps.AddCap(media::VideoRes::RES_640x360);

	ISinkPin* sink_pin = (ISinkPin*)QI(CID_SINK_PIN, IID_SINK_PIN, m_ele_name);
	if (!sink_pin) {
		LOG_ERR("Create sink pin failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != sink_pin->Init(media::MediaType::VIDEO,
			this,
			CONSTRUCT_PIN_NAME("v-sink-pin-"),
			media::util::ToVideoCapsStr(video_pin_caps),
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
ErrCode VideoMixElement::DoInit(com::IProperty* props)
{
	m_logger = g_logger;

	if (ERR_CODE_OK != CreateSrcPin()) {
		LOG_ERR("Create src pin failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != CreateSinkPin()) {
		LOG_ERR("Create sink pin failed!");
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoMixElement::OnSinkPinData(ISinkPin* pin, const PinData& data)
{
	if (m_ele_state != EleState::RUNNING) {
		LOG_ERR("Element is not running!");
		return ERR_CODE_FAILED;
	}

	return SRC_PIN->OnPinData(data);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoMixElement::OnSinkPinNegotiated(ISinkPin* pin, const std::string& cap)
{
	LOG_INF("Sink pin negotiated, cap:{}", cap);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoMixElement::OnSrcPinNegotiated(ISrcPin* pin, const std::string& cap)
{
	LOG_INF("Src pin negotiated, cap:{}", cap);

	return ERR_CODE_OK;
}

}