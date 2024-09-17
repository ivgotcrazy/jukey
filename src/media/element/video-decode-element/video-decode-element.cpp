#include "video-decode-element.h"
#include "util-streamer.h"
#include "util-ffmpeg.h"
#include "common/util-common.h"
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
VideoDecodeElement::VideoDecodeElement(base::IComFactory* factory,
	const char* owner)
	: base::ProxyUnknown(nullptr)
  , base::ComObjTracer(factory, CID_VIDEO_DECODE, owner)
	, ElementBase(factory)
{
	m_ele_name   = CONSTRUCT_ELEMENT_NAME("v-decode-");
	m_main_type  = EleMainType::FILTER;
	m_sub_type   = EleSubType::DECODER;
	m_media_type = EleMediaType::VIDEO_ONLY;

	if (ERR_CODE_OK != CreateSrcPin() || ERR_CODE_OK != CreateSinkPin()) {
		m_pins_created = false;
	}

	m_frame = av_frame_alloc();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
VideoDecodeElement::~VideoDecodeElement()
{
	LOG_INF("Destruct {}", m_ele_name);

	if (m_frame) {
		av_frame_free(&m_frame);
		m_frame = nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* VideoDecodeElement::CreateInstance(base::IComFactory* factory, 
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_VIDEO_DECODE) == 0) {
		return new VideoDecodeElement(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* VideoDecodeElement::NDQueryInterface(const char* riid)
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
ErrCode VideoDecodeElement::CreateSrcPin()
{
	media::com::VideoCaps src_pin_caps;
	src_pin_caps.AddCap(media::VideoCodec::RAW);
	src_pin_caps.AddCap(media::PixelFormat::I420);
	src_pin_caps.AddCap(media::PixelFormat::NV12);
	src_pin_caps.AddCap(media::PixelFormat::YUY2);
	src_pin_caps.AddCap(media::PixelFormat::RGB24);
	src_pin_caps.AddCap(media::VideoRes::RES_1920x1080);
	src_pin_caps.AddCap(media::VideoRes::RES_1280x720);
	src_pin_caps.AddCap(media::VideoRes::RES_640x480);
	src_pin_caps.AddCap(media::VideoRes::RES_640x360);
	src_pin_caps.AddCap(media::VideoRes::RES_320x240);
	src_pin_caps.AddCap(media::VideoRes::RES_320x180);

	ISrcPin* src_pin = (ISrcPin*)QI(CID_SRC_PIN, IID_SRC_PIN, m_ele_name);
	if (!src_pin) {
		LOG_ERR("Create src pin failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != src_pin->Init(media::MediaType::VIDEO,
			this,
			CONSTRUCT_PIN_NAME("v-src-pin-"),
			ToVideoCapsStr(src_pin_caps),
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
ErrCode VideoDecodeElement::CreateSinkPin()
{
	media::com::VideoCaps sink_pin_caps;
	sink_pin_caps.AddCap(media::VideoCodec::H264);
	sink_pin_caps.AddCap(media::PixelFormat::I420);
	sink_pin_caps.AddCap(media::PixelFormat::NV12);
	sink_pin_caps.AddCap(media::PixelFormat::YUY2);
	sink_pin_caps.AddCap(media::PixelFormat::RGB24);
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
			ToVideoCapsStr(sink_pin_caps),
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
void VideoDecodeElement::ParseElementConfig()
{
	try {
		YAML::Node root = YAML::LoadFile(ELEMENT_CONFIG_FILE);

		auto node = root["video-decode-element"];
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

		if (node["log-level"]) {
			g_logger->SetLogLevel(node["log-level"].as<uint32_t>());
		}
	}
	catch (const std::exception& e) {
		LOG_WRN("Error:{}", e.what());
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoDecodeElement::DoInit(com::IProperty* props)
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
ErrCode VideoDecodeElement::DoStart()
{
	LOG_INF("DoStart");

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoDecodeElement::DoPause()
{
	LOG_INF("DoPause");

	DestroyDecoder();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoDecodeElement::DoResume()
{
	LOG_INF("DoResume");

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoDecodeElement::DoStop()
{
	LOG_INF("DoStop");

	DestroyDecoder();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoDecodeElement::OnSinkPinData(ISinkPin* pin, const PinData& data)
{
	LOG_DBG("Received sink pin data:{}", data.media_data[0].data_len);

	if (m_bd_dumper) {
		m_bd_dumper->WriteStreamData(data);
	}

	if (m_ele_state != EleState::RUNNING) {
		LOG_ERR("Invalid element state:{}!", m_ele_state);
		return ERR_CODE_FAILED;
	}

	if (!m_codec_ctx) {
		LOG_ERR("Invalid codec context!");
		return ERR_CODE_FAILED;
	}

	AVPacket packet = {0};
	packet.data = (uint8_t*)data.media_data[0].data.get();
	packet.size = data.media_data[0].data_len;
	packet.dts  = data.dts;
	packet.pts  = data.pts;
	packet.duration = data.drt;

	int ret = avcodec_send_packet(m_codec_ctx, &packet);
	if (ret != 0) {
		char err_str[1024];
		av_strerror(ret, err_str, sizeof(err_str));
		LOG_ERR("avcodec_send_packet failed, ret:{}, err:{}", ret, err_str);
		av_packet_unref(&packet);
		return ERR_CODE_FAILED;
	}

	while (true) {
		int ret = avcodec_receive_frame(m_codec_ctx, m_frame);
		if (ret != 0) {
			if (ret != AVERROR(EAGAIN)) {
				char err_str[1024];
				av_strerror(ret, err_str, sizeof(err_str));
				LOG_ERR("avcodec_receive_frame failed, ret:{}, err:{}", ret, err_str);
			}
			break;
		}

		PinData decoded_data(media::MediaType::VIDEO);
		decoded_data.dts = m_frame->pkt_dts;
		decoded_data.pts = m_frame->pts;
		decoded_data.drt = m_frame->pkt_duration;
		decoded_data.pos = m_frame->pkt_pos;
		decoded_data.syn = data.syn;
		decoded_data.tbn = data.tbn;
		decoded_data.tbd = data.tbd;

		decoded_data.media_data[0].data.reset(m_frame->data[0], 
			util::NoDestruct);
		decoded_data.media_data[0].data_len = 
			m_frame->linesize[0] * m_frame->height;
		decoded_data.media_data[1].data.reset(m_frame->data[1], 
			util::NoDestruct);
		decoded_data.media_data[1].data_len = 
			m_frame->linesize[1] * m_frame->height / 2;
		decoded_data.media_data[2].data.reset(m_frame->data[2], 
			util::NoDestruct);
		decoded_data.media_data[2].data_len = 
			m_frame->linesize[2] * m_frame->height / 2;

		decoded_data.data_count = 3;

		if (com::ERR_CODE_OK != SRC_PIN->OnPinData(decoded_data)) {
			LOG_ERR("OnPinData failed!");
		}

		if (m_ad_dumper) {
			m_ad_dumper->WriteStreamData(decoded_data);
		}

		av_frame_unref(m_frame);
	}
	av_packet_unref(&packet);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoDecodeElement::UpdatePinCap(const std::string& src_cap,
	const std::string& sink_cap)
{
	if (!src_cap.empty()) {
		auto video_src_cap = media::util::ParseVideoCap(src_cap);
		if (video_src_cap.has_value()) {
			m_src_pin_cap = video_src_cap.value();
		}
		else {
			LOG_ERR("Parse cap failed, cap:{}", src_cap);
		}
	}
	
	if (!sink_cap.empty()) {
		auto video_sink_cap = media::util::ParseVideoCap(sink_cap);
		if (video_sink_cap.has_value()) {
			m_sink_pin_cap = video_sink_cap.value();
		}
		else {
			LOG_ERR("Parse cap failed, cap:{}", sink_cap);
		}
	}

	TryCreateDecoder();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode 
VideoDecodeElement::OnSinkPinNegotiated(ISinkPin* pin, const std::string& cap)
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
		MatchVideoPinCapsWithoutCodec(SRC_PIN->PrepCaps(), cap))) {
		LOG_ERR("Update src pin available caps failed");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != SRC_PIN->Negotiate()) {
		LOG_ERR("Src pin negotiate failed");
		return ERR_CODE_FAILED;
	}

	LOG_INF("Src pin negotiated cap:{}", Capper(SRC_PIN->Cap()));

	UpdatePinCap(SRC_PIN->Cap(), cap);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode
VideoDecodeElement::OnSrcPinNegotiated(ISrcPin* pin, const std::string& cap)
{
	LOG_INF("src pin negotiated, cap:{}", Capper(cap));

	if (!pin) {
		LOG_ERR("Invalid pin");
		return ERR_CODE_INVALID_PARAM;
	}

	UpdatePinCap(cap, SINK_PIN->Cap());

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// TODO: repeatedly create
//------------------------------------------------------------------------------
ErrCode VideoDecodeElement::TryCreateDecoder()
{
	if (!SINK_PIN->Negotiated() || !SRC_PIN->Negotiated()) {
		LOG_INF("Element not negotiated, cannot create decoder");
		return ERR_CODE_OK;
	}

	m_codec = avcodec_find_decoder(ToFfVideoCodec(m_sink_pin_cap.codec));
	if (!m_codec) {
		LOG_ERR("avcodec_find_decoder:{} failed!", m_sink_pin_cap.codec);
		return ERR_CODE_FAILED;
	}

	m_codec_ctx = avcodec_alloc_context3(m_codec);
	if (!m_codec_ctx) {
		LOG_ERR("avcodec_alloc_context3 failed!");
		return ERR_CODE_FAILED;
	}

	m_codec_ctx->err_recognition = AV_EF_IGNORE_ERR;
	m_codec_ctx->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK
		| FF_EC_FAVOR_INTER;

	int result = avcodec_open2(m_codec_ctx, m_codec, NULL);
	if (result < 0) {
		LOG_ERR("avcodec_open2 failed, error:{}", result);
		return ERR_CODE_FAILED;
	}

	LOG_INF("Create decoder:{} success", m_sink_pin_cap.codec);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoDecodeElement::DestroyDecoder()
{
	LOG_INF("Destroy decoder!");

	if (m_codec_ctx) {
		avcodec_close(m_codec_ctx);
		avcodec_free_context(&m_codec_ctx);
		m_codec_ctx = nullptr;
	}
}

}