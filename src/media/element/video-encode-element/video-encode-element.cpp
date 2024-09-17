#include "video-encode-element.h"
#include "util-streamer.h"
#include "common/util-common.h"
#include "util-x264.h"
#include "util-enum.h"
#include "util-streamer.h"
#include "log.h"
#include "common/media-common-define.h"
#include "yaml-cpp/yaml.h"
#include "common/util-time.h"


using namespace jukey::com;
using namespace jukey::media::util;


namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
VideoEncodeElement::VideoEncodeElement(base::IComFactory* factory,
	const char* owner)
	: base::ProxyUnknown(nullptr)
  , base::ComObjTracer(factory, CID_VIDEO_ENCODE, owner)
	, ElementBase(factory)
{
	m_ele_name   = CONSTRUCT_ELEMENT_NAME("v-encode-");
	m_main_type  = EleMainType::FILTER;
	m_sub_type   = EleSubType::ENCODER;
	m_media_type = EleMediaType::VIDEO_ONLY;

	if (ERR_CODE_OK != CreateSrcPin() || ERR_CODE_OK != CreateSinkPin()) {
		m_pins_created = false;
	}

	x264_picture_init(&m_out_pic);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
VideoEncodeElement::~VideoEncodeElement()
{
	LOG_INF("Destruct {}", m_ele_name);

	DoStop();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* VideoEncodeElement::CreateInstance(base::IComFactory* factory, 
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_VIDEO_ENCODE) == 0) {
		return new VideoEncodeElement(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* VideoEncodeElement::NDQueryInterface(const char* riid)
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
com::ErrCode VideoEncodeElement::ProcSinkPinMsg(stmr::ISinkPin* pin,
	const stmr::PinMsg& msg)
{
	LOG_INF("Proc sink pin msg:{}", msg.msg_type);

	if (!m_br_alloc_mgr) return ERR_CODE_OK;

	if (msg.msg_type == stmr::PinMsgType::SET_STREAM) {
		if (!m_stream_id.empty()) {
			m_br_alloc_mgr->UnregisterListener(m_stream_id, this);
		}
		m_stream_id = msg.msg_data.sp;
		m_br_alloc_mgr->RegsiterListener(m_stream_id, this);
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoEncodeElement::CreateSrcPin()
{
	media::com::VideoCaps src_pin_caps;
	src_pin_caps.AddCap(media::VideoCodec::H264);

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
			CONSTRUCT_PIN_NAME("video-src-pin-"),
			media::util::ToVideoCapsStr(src_pin_caps),
			this)) {
		LOG_ERR("Init video src pin failed!");
		return ERR_CODE_FAILED;
	}

	m_src_pin_index = static_cast<uint32_t>(m_src_pins.size());
	m_src_pins.push_back(src_pin);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoEncodeElement::CreateSinkPin()
{
	media::com::VideoCaps sink_pin_caps;
	sink_pin_caps.AddCap(media::VideoCodec::RAW);

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

	ISinkPin*sink_pin = (ISinkPin*)QI(CID_SINK_PIN, IID_SINK_PIN, m_ele_name);
	if (!sink_pin) {
		LOG_ERR("Create sink pin failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != sink_pin->Init(media::MediaType::VIDEO,
			this,
			CONSTRUCT_PIN_NAME("video-sink-pin-"),
			media::util::ToVideoCapsStr(sink_pin_caps),
			this)) {
		LOG_ERR("Init video sink pin failed!");
		return ERR_CODE_FAILED;
	}

	m_sink_pin_index = static_cast<uint32_t>(m_sink_pins.size());
	m_sink_pins.push_back(sink_pin);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoEncodeElement::ParseElementConfig()
{
	try {
		YAML::Node root = YAML::LoadFile(ELEMENT_CONFIG_FILE);

		auto node = root["video-encode-element"];
		if (!node) {
			return;
		}

		if (node["dump-encoded-data"]) {
			if (node["dump-encoded-data"].as<bool>()) {
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
ErrCode VideoEncodeElement::ParseProperties(com::IProperty* props)
{
	if (!props) {
		LOG_INF("No properties");
		return ERR_CODE_OK;
	}

	m_br_alloc_mgr = (IBitrateAllocateMgr*)props->GetPtrValue(
		"bitrate-allocate-mgr");
	if (!m_br_alloc_mgr) {
		LOG_INF("No birate allocate manager");
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoEncodeElement::DoInit(com::IProperty* props)
{
	m_logger = g_logger;

	ParseElementConfig();

	ParseProperties(props);

	m_data_stats.reset(new util::DataStats(m_factory, g_logger, m_ele_name));
	m_data_stats->Start();

	util::StatsParam br_stats("bitrate", util::StatsType::IAVER, 5000);
	m_br_stats_id = m_data_stats->AddStats(br_stats);

	util::StatsParam fr_stats("framerate", util::StatsType::IAVER, 5000);
	m_fr_stats_id = m_data_stats->AddStats(fr_stats);

	// TODO: 查表确定
	m_curr_bitrate_kbps = 4000;
	m_last_update_us = util::Now();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoEncodeElement::DoStart()
{
	LOG_INF("DoStart");

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoEncodeElement::DoPause()
{
	LOG_INF("DoPause");

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoEncodeElement::DoResume()
{
	LOG_INF("DoResume");

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoEncodeElement::DoStop()
{
	LOG_INF("DoStop");

	m_data_stats->Stop();

	if (m_x264_encoder) {
		x264_encoder_close(m_x264_encoder);
		m_x264_encoder = nullptr;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoEncodeElement::CreateEncoder()
{
	LOG_INF("Create encoder, birate:{}", m_curr_bitrate_kbps);

	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_x264_encoder) {
		x264_encoder_close(m_x264_encoder);
		m_x264_encoder = nullptr;
	}

	x264_param_t x264_param;

	/* Get default params for preset/tuning */
	if (x264_param_default_preset(&x264_param, "medium", NULL) < 0) {
		LOG_ERR("x264_param_default_preset 'medium' failed!");
		return ERR_CODE_FAILED;
	}

	if (x264_param_default_preset(&x264_param, "fast", "zerolatency") < 0) {
		LOG_ERR("x264_param_default_preset 'fast' 'zerolatency' failed!");
		return ERR_CODE_FAILED;
	}

	/* Configure non-default params */
	x264_param.i_bitdepth = 8;
	x264_param.i_bframe = 0;
	x264_param.i_csp = media::util::ToX264PixelFormat(m_sink_pin_cap.format);
	x264_param.i_width = media::util::GetWidth(m_sink_pin_cap.res);
	x264_param.i_height = media::util::GetHeight(m_sink_pin_cap.res);
	x264_param.i_timebase_num = 1;
	x264_param.i_timebase_den = 50;
	x264_param.i_fps_den = 1;
	x264_param.i_fps_num = 25;
	x264_param.i_threads = 4;

	x264_param.b_vfr_input = 0; // TODO: 设置为1，则rc设置无效
	x264_param.b_repeat_headers = 1;
	x264_param.b_annexb = 1;

	// Bitrate
	x264_param.rc.i_rc_method = X264_RC_ABR;
	x264_param.rc.i_bitrate = m_curr_bitrate_kbps;
	x264_param.rc.i_vbv_max_bitrate = m_curr_bitrate_kbps;
	x264_param.rc.f_rate_tolerance = 1.0;

	// GOP
	x264_param.i_keyint_max = 30;
	x264_param.i_keyint_min = 30;

	/* Apply profile restrictions. */
	if (x264_param_apply_profile(&x264_param, "high") < 0) {
		LOG_ERR("x264_param_apply_profile failed!");
		return ERR_CODE_FAILED;
	}

	m_x264_encoder = x264_encoder_open(&x264_param);
	if (!m_x264_encoder) {
		LOG_ERR("Create x264 encoder failed!");
		return ERR_CODE_FAILED;
	}

	LOG_INF("Create x264 encoder success, format:{}, resolution:{}",
		media::util::VIDEO_FMT_STR(m_sink_pin_cap.format),
		media::util::VIDEO_RES_STR(m_sink_pin_cap.res));

	if (x264_picture_alloc(&m_in_pic, x264_param.i_csp, x264_param.i_width,
		x264_param.i_height) < 0) {
		LOG_ERR("x264_picture_alloc failed!");
		return ERR_CODE_FAILED;
	}
	
	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoEncodeElement::FillX264InPicture(const PinData& data)
{
	if (data.data_count == 1) { // packed
		if (m_sink_pin_cap.format == media::PixelFormat::I420) {
			m_in_pic.img.plane[0] = data.media_data[0].data.get();
			m_in_pic.img.plane[1] = data.media_data[0].data.get() 
				+ data.media_data[0].data_len * 2 / 3;
			m_in_pic.img.plane[2] = data.media_data[0].data.get() 
				+ data.media_data[0].data_len * 3 / 4;

			m_in_pic.img.i_stride[0] = media::util::GetWidth(m_sink_pin_cap.res);
			m_in_pic.img.i_stride[1] = media::util::GetWidth(m_sink_pin_cap.res) / 2;
			m_in_pic.img.i_stride[2] = media::util::GetWidth(m_sink_pin_cap.res) / 2;

			m_in_pic.img.i_plane = 3;
		}
		else if (m_sink_pin_cap.format == media::PixelFormat::NV12) {
			m_in_pic.img.plane[0] = data.media_data[0].data.get();
			m_in_pic.img.plane[1] = data.media_data[0].data.get() 
				+ data.media_data[0].data_len / 2;
			m_in_pic.img.plane[2] = 0;

			m_in_pic.img.i_stride[0] = media::util::GetWidth(m_sink_pin_cap.res);
			m_in_pic.img.i_stride[1] = media::util::GetWidth(m_sink_pin_cap.res);
			m_in_pic.img.i_stride[2] = 0;

			m_in_pic.img.i_plane = 2;
		}
		else {
			LOG_ERR("Unsupported pixel format:{}", m_sink_pin_cap.format);
			return ERR_CODE_FAILED;
		}
	}
	else { // planar
		if (m_sink_pin_cap.format == media::PixelFormat::I420) {
			m_in_pic.img.plane[0] = data.media_data[0].data.get();
			m_in_pic.img.plane[1] = data.media_data[1].data.get();
			m_in_pic.img.plane[2] = data.media_data[2].data.get();

			m_in_pic.img.i_stride[0] = media::util::GetWidth(m_sink_pin_cap.res);
			m_in_pic.img.i_stride[1] = media::util::GetWidth(m_sink_pin_cap.res) / 2;
			m_in_pic.img.i_stride[2] = media::util::GetWidth(m_sink_pin_cap.res) / 2;

			m_in_pic.img.i_plane = 3;
		}
		else if (m_sink_pin_cap.format == media::PixelFormat::NV12) {
			m_in_pic.img.plane[0] = data.media_data[0].data.get();
			m_in_pic.img.plane[1] = data.media_data[1].data.get();
			m_in_pic.img.plane[2] = 0;

			m_in_pic.img.i_stride[0] = media::util::GetWidth(m_sink_pin_cap.res);
			m_in_pic.img.i_stride[1] = media::util::GetWidth(m_sink_pin_cap.res);
			m_in_pic.img.i_stride[2] = 0;

			m_in_pic.img.i_plane = 2;
		}
		else {
			LOG_ERR("Unsupported pixel format:{}", m_sink_pin_cap.format);
			return ERR_CODE_FAILED;
		}
	}

	m_in_pic.i_pts = data.pts;
	m_in_pic.i_dts = data.dts;

	return ERR_CODE_OK;
}


//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode VideoEncodeElement::OnSinkPinData(ISinkPin* pin, const PinData& data)
{
	LOG_DBG("OnSinkPinData, count:{}, len1:{}, len2:{}, len3:{}", 
		data.data_count, 
		data.media_data[0].data_len,
		data.media_data[1].data_len,
		data.media_data[2].data_len);

	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_ele_state != EleState::RUNNING) {
		LOG_ERR("Element is not running!");
		return ERR_CODE_FAILED;
	}

	if (com::ERR_CODE_OK != FillX264InPicture(data)) {
		LOG_ERR("FillX264InPicture failed!");
		return ERR_CODE_FAILED;
	}

	x264_nal_t* nal_data = nullptr;
	int nal_count = 0;
	int frame_size = x264_encoder_encode(m_x264_encoder, 
		&nal_data, 
		&nal_count,
		&m_in_pic, 
		&m_out_pic);
	if (frame_size < 0) {
		LOG_ERR("Encode failed, frame_size:{}", frame_size);
		return ERR_CODE_FAILED;
	}
	else if (frame_size == 0) {
		LOG_WRN("No nalu returned!");
		return ERR_CODE_OK;
	}
	
	ErrCode result = ERR_CODE_OK;
	if (m_send_single_nalu) { // nalu by nalu
		for (x264_nal_t* nal = nal_data; nal < nal_data + nal_count; nal++) {
			PinData encoded_data(media::MediaType::VIDEO, nal->p_payload,
				nal->i_payload);

			encoded_data.pts = m_out_pic.i_pts;
			encoded_data.dts = m_out_pic.i_dts;
			encoded_data.drt = data.drt;
			encoded_data.pos = 0;
			encoded_data.tbn = 1; // data.tbn;
			encoded_data.tbd = 50;// data.tbd;
			encoded_data.data_count = 1;

			encoded_data.media_para = data.media_para;
			auto para = SPC<media::VideoFramePara>(encoded_data.media_para);
			para->codec = m_src_pin_cap.codec;

			m_data_stats->OnData(m_br_stats_id, nal->i_payload);
			m_data_stats->OnData(m_fr_stats_id, 1);

			result = SRC_PIN->OnPinData(encoded_data);
			if (com::ERR_CODE_OK != result) {
				LOG_WRN("Send data failed!");
				break;
			}
			if (m_stream_dumper) {
				m_stream_dumper->WriteStreamData(encoded_data);
			}
		}
	}
	else { // all in one
		PinData encoded_data(media::MediaType::VIDEO, 
			nal_data->p_payload, frame_size);

		encoded_data.pts = m_out_pic.i_pts;
		encoded_data.dts = m_out_pic.i_dts;
		encoded_data.drt = data.drt;
		encoded_data.pos = 0;
		encoded_data.tbn = data.tbn;
		encoded_data.tbd = data.tbd;
		encoded_data.data_count = 1;

		encoded_data.media_para = data.media_para;
		auto para = SPC<media::VideoFramePara>(encoded_data.media_para);
		para->codec = m_src_pin_cap.codec;
		para->width = media::util::GetWidth(m_src_pin_cap.res);
		para->height = media::util::GetHeight(m_src_pin_cap.res);
		para->seq = ++m_frame_seq;

		m_data_stats->OnData(m_br_stats_id, frame_size);
		m_data_stats->OnData(m_fr_stats_id, 1);

		result = SRC_PIN->OnPinData(encoded_data);

		if (m_stream_dumper) {
			m_stream_dumper->WriteStreamData(encoded_data);
		}
	}

	return result;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoEncodeElement::UpdatePinCap(const std::string& src_cap,
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
	
	CreateEncoder();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode 
VideoEncodeElement::OnSinkPinNegotiated(ISinkPin* pin, const std::string& cap)
{
	LOG_INF("Sink pin negotiated, cap:{}", media::util::Capper(cap));

	if (!pin) {
		LOG_ERR("Invalid pin");
		return ERR_CODE_INVALID_PARAM;
	}

	assert(SRC_PIN);

	if (SRC_PIN->PrepCaps().empty()) {
		LOG_ERR("Empty src pin prepared caps");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != SRC_PIN->UpdateAvaiCaps(
		media::util::MatchVideoPinCapsWithoutCodec(SRC_PIN->PrepCaps(), cap))) {
		LOG_ERR("Update src pin available caps failed");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != SRC_PIN->Negotiate()) {
		LOG_ERR("Src pin negotiate failed");
		return ERR_CODE_FAILED;
	}

	LOG_INF("Src pin negotiated cap:{}", media::util::Capper(SRC_PIN->Cap()));

	UpdatePinCap(SRC_PIN->Cap(), cap);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode 
VideoEncodeElement::OnSrcPinNegotiated(ISrcPin* pin, const std::string& cap)
{
	LOG_INF("Src pin negotiated, cap:{}", media::util::Capper(cap));

	if (!pin) {
		LOG_ERR("Invalid pin");
		return ERR_CODE_INVALID_PARAM;
	}

	UpdatePinCap(cap, SINK_PIN->Cap());

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoEncodeElement::UpdateBitrateList(uint32_t br_kbps)
{
	if (m_bitrate_list.size() >= 10) {
		m_bitrate_list.pop_front();
	}
	m_bitrate_list.push_back(br_kbps);
}

//------------------------------------------------------------------------------
// 最近10次均值高于当前码率20%以上，且最近一次码率大于平均值，则升码率
//------------------------------------------------------------------------------
std::optional<uint32_t> VideoEncodeElement::TryIncreaseBitrate()
{
	std::optional<uint32_t> result;

	if (m_bitrate_list.size() >= 10 && m_curr_bitrate_kbps < 4000) {
		uint32_t total = 0;
		uint32_t max = 0;
		uint32_t min = 0xFFFFFFFF;
		for (auto br : m_bitrate_list) {
			total += br;
			if (br > max) max = br;
			if (br < min) min = br;
		}

		// 去掉最大值和最小值
		uint32_t last_10_aver = (total - max - min) / 8;

		if (last_10_aver > m_curr_bitrate_kbps * 1.2 
			&& m_bitrate_list.back() >= last_10_aver) {
			if (last_10_aver > 4000) {
				result.emplace(4000);
			}
			else {
				result.emplace(last_10_aver);
			}
		}
	}

	return result;
}

//------------------------------------------------------------------------------
// 最近3次均值低于当前码率20%以上，且最近一次码率小于平均值，则降码率
//------------------------------------------------------------------------------
std::optional<uint32_t> VideoEncodeElement::TryDecreaseBitrate()
{
	std::optional<uint32_t> result;

	//if (m_bitrate_list.size() >= 5) {
	//	int count = 5;
	//	uint32_t total = 0;
	//	uint32_t max = 0;
	//	uint32_t min = 0xFFFFFFFF;
	//	for (auto it = m_bitrate_list.begin(); it != m_bitrate_list.end(); it++) {
	//		total += *it;
	//		if (*it > max) max = *it;
	//		if (*it < min) min = *it;
	//		if (--count <= 0) break;
	//	}

	//	uint32_t last_3_aver = (total - max - min) / 3;

	//	if (last_3_aver <= m_curr_bitrate_kbps * 0.8
	//		&& m_bitrate_list.back() < last_3_aver) {
	//		result.emplace(last_3_aver);
	//	}
	//}
	
	if (!m_bitrate_list.empty()) {
		if (m_bitrate_list.back() <= m_curr_bitrate_kbps * 0.9) {
			result.emplace(m_bitrate_list.back());
		}
	}
	

	return result;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoEncodeElement::UpdateBitrate(uint32_t br_kbps)
{
	UpdateBitrateList(br_kbps);
	
	bool update = false;

	auto decrease_result = TryDecreaseBitrate();
	if (decrease_result.has_value()
		/*&& util::Now() > m_last_update_us + kDecreaseIntervalMs * 1000*/) {
		m_curr_bitrate_kbps = decrease_result.value();
		m_last_update_us = util::Now();
		update = true;
	}
	else {
		auto increase_result = TryIncreaseBitrate();
		if (increase_result.has_value()
			&& util::Now() > m_last_update_us + kIncreaseIntervalMs * 1000) {
			m_curr_bitrate_kbps = increase_result.value();
			m_last_update_us = util::Now();
			update = true;
		}
	}

	if (update) CreateEncoder();
}

}