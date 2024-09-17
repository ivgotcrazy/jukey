#include "video-convert-element.h"
#include "util-sdl.h"
#include "util-ffmpeg.h"
#include "util-streamer.h"
#include "common/util-common.h"
#include "common/media-common-define.h"


using namespace jukey::com;

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// Construction
//------------------------------------------------------------------------------
VideoConvertElement::VideoConvertElement(base::IComFactory* factory,
	const char* owner)
	: ProxyUnknown(nullptr)
  , ComObjTracer(factory, CID_VIDEO_CONVERT, owner)
	, ElementBase(factory)
{
	m_ele_name   = CONSTRUCT_ELEMENT_NAME("v-convert-");
	m_main_type  = EleMainType::FILTER;
	m_sub_type   = EleSubType::CONVERTER;
	m_media_type = EleMediaType::VIDEO_ONLY;

	if (ERR_CODE_OK != CreateSrcPin() || ERR_CODE_OK != CreateSinkPin()) {
		m_pins_created = false;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
VideoConvertElement::~VideoConvertElement()
{
	LOG_INF("Destruct {}", m_ele_name);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* VideoConvertElement::CreateInstance(base::IComFactory* factory, 
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_VIDEO_CONVERT) == 0) {
		return new VideoConvertElement(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* VideoConvertElement::NDQueryInterface(const char* riid)
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
ErrCode VideoConvertElement::CreateSrcPin()
{
	media::com::VideoCaps src_pin_caps;
	src_pin_caps.AddCap(media::VideoCodec::RAW);
	src_pin_caps.AddCap(media::PixelFormat::I420);
	src_pin_caps.AddCap(media::PixelFormat::YUY2);
	src_pin_caps.AddCap(media::PixelFormat::NV12);
	src_pin_caps.AddCap(media::PixelFormat::RGB24);
	src_pin_caps.AddCap(media::VideoRes::RES_1920x1080);
	src_pin_caps.AddCap(media::VideoRes::RES_1280x720);
	src_pin_caps.AddCap(media::VideoRes::RES_640x480);
	src_pin_caps.AddCap(media::VideoRes::RES_640x360);

	ISrcPin* src_pin = (ISrcPin*)QI(CID_SRC_PIN, IID_SRC_PIN, m_ele_name);
	if (!src_pin) {
		LOG_ERR("Create src pin failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != src_pin->Init(media::MediaType::VIDEO,
			this,
			CONSTRUCT_PIN_NAME("v-src-pin-"),
			media::util::ToVideoCapsStr(src_pin_caps),
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
ErrCode VideoConvertElement::CreateSinkPin()
{
	media::com::VideoCaps sink_pin_caps;
	sink_pin_caps.AddCap(media::VideoCodec::RAW);
	sink_pin_caps.AddCap(media::PixelFormat::I420);
	sink_pin_caps.AddCap(media::PixelFormat::YUY2);
	sink_pin_caps.AddCap(media::PixelFormat::NV12);
	sink_pin_caps.AddCap(media::PixelFormat::RGB24);
	sink_pin_caps.AddCap(media::VideoRes::RES_1920x1080);
	sink_pin_caps.AddCap(media::VideoRes::RES_1280x720);
	sink_pin_caps.AddCap(media::VideoRes::RES_640x480);
	sink_pin_caps.AddCap(media::VideoRes::RES_640x360);

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
ErrCode VideoConvertElement::DoInit(com::IProperty* props)
{
	m_logger = g_logger;

	av_log_set_callback(LogCallback);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoConvertElement::TryCreateSwsContext()
{
	if (SRC_PIN->Negotiated() && SINK_PIN->Negotiated()) {
		m_need_convert = !(m_src_pin_cap == m_sink_pin_cap);
		if (m_need_convert) {
			if (ERR_CODE_OK != CreateSwsContext()) {
				LOG_ERR("Create sws context failed!");
			}
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoConvertElement::DoStart()
{
	TryCreateSwsContext();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoConvertElement::DoPause()
{
	DestroySwsContext();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoConvertElement::DoResume()
{
	TryCreateSwsContext();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoConvertElement::DoStop()
{
	DestroySwsContext();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoConvertElement::SetSrcDataAndLineSize(const PinData& data, 
	uint8_t** src_data, int* linesize)
{
	if (data.data_count == 1) {
		if (m_sink_pin_cap.format == media::PixelFormat::NV12) {
			src_data[0] = DP(data.media_data[0]);
			src_data[1] = DP(data.media_data[0]) + data.media_data[0].data_len * 2 / 3;

			linesize[0] = media::util::GetWidth(m_sink_pin_cap.res);
			linesize[1] = media::util::GetWidth(m_sink_pin_cap.res);
		}
		else if (m_sink_pin_cap.format == media::PixelFormat::I420) {
			src_data[0] = DP(data.media_data[0]);
			src_data[1] = DP(data.media_data[0]) + data.media_data[0].data_len * 2 / 3;
			src_data[2] = DP(data.media_data[0]) + data.media_data[0].data_len * 5 / 6;

			linesize[0] = media::util::GetWidth(m_sink_pin_cap.res);
			linesize[1] = media::util::GetWidth(m_sink_pin_cap.res) / 2;
			linesize[2] = media::util::GetWidth(m_sink_pin_cap.res) / 2;
		}
		else if (m_sink_pin_cap.format == media::PixelFormat::YUY2) {
			src_data[0] = DP(data.media_data[0]);
			linesize[0] = media::util::GetWidth(m_sink_pin_cap.res) * 3 / 2;
		}
		else if (m_sink_pin_cap.format == media::PixelFormat::RGB24) {
			src_data[0] = DP(data.media_data[0]);
			linesize[0] = media::util::GetWidth(m_sink_pin_cap.res) * 3;
		}
		else {
			LOG_ERR("Unsupport pixel format:{}", m_sink_pin_cap.format);
			return ERR_CODE_FAILED;
		}
	}
	else {
		if (m_sink_pin_cap.format == media::PixelFormat::NV12) {
			src_data[0] = DP(data.media_data[0]);
			src_data[1] = DP(data.media_data[1]);

			linesize[0] = media::util::GetWidth(m_sink_pin_cap.res);
			linesize[1] = media::util::GetWidth(m_sink_pin_cap.res);
		}
		else if (m_sink_pin_cap.format == media::PixelFormat::I420) {
			src_data[0] = DP(data.media_data[0]);
			src_data[1] = DP(data.media_data[1]);
			src_data[2] = DP(data.media_data[2]);

			linesize[0] = media::util::GetWidth(m_sink_pin_cap.res);
			linesize[1] = media::util::GetWidth(m_sink_pin_cap.res) / 2;
			linesize[2] = media::util::GetWidth(m_sink_pin_cap.res) / 2;
		}
		else {
			LOG_ERR("Unsupport pixel format:{}", m_sink_pin_cap.format);
			return ERR_CODE_FAILED;
		}
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoConvertElement::ConvertPinData(const PinData& data)
{
	if (!m_sws_ctx) {
		return ERR_CODE_OK;
	}

	uint8_t* src_data[8] = { 0 };
	int src_linesize[8] = { 0 };
	if (ERR_CODE_OK != SetSrcDataAndLineSize(data, src_data, src_linesize)) {
		return ERR_CODE_FAILED;
	}

	// pre-caculate
	uint32_t height = media::util::GetHeight(m_src_pin_cap.res);
	uint32_t width = media::util::GetWidth(m_src_pin_cap.res);
	AVPixelFormat format = media::util::ToFfPixelFormat(m_src_pin_cap.format);

	int buf_size = av_image_get_buffer_size(format, width, height, 1);
	std::shared_ptr<uint8_t> buf(new uint8_t[buf_size]);

	uint8_t* dst_data[8] = { 0 };
	int dst_linesize[8] = { 0 };
	av_image_fill_arrays(dst_data, dst_linesize, buf.get(), format, width, height, 1);
	
	int result = sws_scale(m_sws_ctx, 
		(uint8_t const* const*)src_data,
		src_linesize,
		0, 
		media::util::GetHeight(m_sink_pin_cap.res),
		dst_data, 
		dst_linesize);
	if (result <= 0) {
		LOG_ERR("Call sws_scale failed, result:{}, error:{}", result);
		return ERR_CODE_FAILED;
	}

	PinData new_data(media::MediaType::VIDEO);
	new_data.dts = data.dts;
	new_data.pts = data.pts;
	new_data.drt = 0;
	new_data.pos = 0;
	new_data.syn = data.syn;
	new_data.tbn = data.tbn;
	new_data.tbd = data.tbd;

	// Copy data
	if (m_use_planar) { // planar
		if (dst_linesize[0] > 0) {
			new_data.media_data[0].data_len = dst_linesize[0] * height;
			new_data.media_data[0].data.reset(dst_data[0], util::NoDestruct);
			new_data.data_count++;
		}

		if (dst_linesize[1] > 0) {
			new_data.media_data[1].data_len = dst_linesize[1] * height / 2;
			new_data.media_data[1].data.reset(dst_data[1], util::NoDestruct);
			new_data.data_count++;
		}

		if (dst_linesize[2] > 0) {
			new_data.media_data[2].data_len = dst_linesize[2] * height / 2;
			new_data.media_data[2].data.reset(dst_data[2], util::NoDestruct);
			new_data.data_count++;
		}
	}
	else { // packed
		new_data.media_data[0].data_len = buf_size;
		new_data.media_data[0].data.reset(buf.get(), util::NoDestruct);
		new_data.data_count = 1;
	}

	// Copy parameters and update capability
	new_data.media_para = data.media_para;
	
	auto para = SPC<media::VideoFramePara>(new_data.media_para);
	para->height = media::util::GetHeight(m_src_pin_cap.res);
	para->width = media::util::GetWidth(m_src_pin_cap.res);
	

	return SRC_PIN->OnPinData(new_data);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoConvertElement::OnSinkPinData(ISinkPin* pin, const PinData& data)
{
	LOG_DBG("OnSinkPinData, len:{}", data.media_data[0].data_len);

	if (m_ele_state != EleState::RUNNING) {
		LOG_DBG("Element is not running!");
		return ERR_CODE_FAILED;
	}

	assert(data.mt == media::MediaType::VIDEO);

	if (m_need_convert) {
		return ConvertPinData(data);
	}
	else {
		return SRC_PIN->OnPinData(data);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoConvertElement::UpdatePinCap(const std::string& src_cap,
	const std::string& sink_cap)
{
	auto video_src_cap = media::util::ParseVideoCap(src_cap);
	if (video_src_cap.has_value()) {
		m_src_pin_cap = video_src_cap.value();
	}

	auto video_sink_cap = media::util::ParseVideoCap(sink_cap);
	if (video_sink_cap.has_value()) {
		m_sink_pin_cap = video_sink_cap.value();
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode 
VideoConvertElement::OnSinkPinNegotiated(ISinkPin* pin, const std::string& cap)
{
	LOG_INF("Sink pin negotiated, cap:{}", media::util::Capper(cap));

	// 这个逻辑最好放到基类进行处理
	if (ErrCode::ERR_CODE_OK != SRC_PIN->Negotiate()) {
		LOG_ERR("Negotiate failed!");
		return ERR_CODE_FAILED;
	}

	UpdatePinCap(SRC_PIN->Cap(), cap);
	TryCreateSwsContext();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode
VideoConvertElement::OnSrcPinNegotiated(ISrcPin* src_pin, const std::string& cap)
{
	LOG_INF("Src pin negotiated, cap:{}", media::util::Capper(cap));

	UpdatePinCap(cap, SINK_PIN->Cap());
	TryCreateSwsContext();

	return ErrCode::ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoConvertElement::CreateSwsContext()
{
	if (media::util::IsInvalidVideoCap(m_sink_pin_cap) 
		|| media::util::IsInvalidVideoCap(m_src_pin_cap)) {
		LOG_WRN("Invalid pin capability!");
		return ERR_CODE_FAILED;
	}

	LOG_INF("Create sws context, src resolution:{}|{}, src format:{}, "
		"dst resolution:{}|:{}, dst format:{}",
		media::util::GetWidth(m_sink_pin_cap.res),
		media::util::GetHeight(m_sink_pin_cap.res),
		media::util::ToFfPixelFormat(m_sink_pin_cap.format),
		media::util::GetWidth(m_src_pin_cap.res),
		media::util::GetHeight(m_src_pin_cap.res),
		media::util::ToFfPixelFormat(m_src_pin_cap.format));

	m_sws_ctx = sws_getContext(
		media::util::GetWidth(m_sink_pin_cap.res),
		media::util::GetHeight(m_sink_pin_cap.res),
		media::util::ToFfPixelFormat(m_sink_pin_cap.format),
		media::util::GetWidth(m_src_pin_cap.res),
		media::util::GetHeight(m_src_pin_cap.res),
		media::util::ToFfPixelFormat(m_src_pin_cap.format),
		SWS_FAST_BILINEAR,
		NULL,
		NULL,
		NULL);
	if (!m_sws_ctx) {
		LOG_ERR("Create sws context failed!");
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// TODO: Repeatedly
//------------------------------------------------------------------------------
void VideoConvertElement::DestroySwsContext()
{
	// TODO: Repeat create
	if (m_sws_ctx) {
		sws_freeContext(m_sws_ctx);
		m_sws_ctx = nullptr;
	}
}

}