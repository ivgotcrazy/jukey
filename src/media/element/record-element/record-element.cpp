#include "record-element.h"
#include "util-streamer.h"
#include "util-ffmpeg.h"
#include "common/util-common.h"
#include "log.h"
#include "common/media-common-define.h"


using namespace jukey::com;

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
RecordElement::RecordElement(base::IComFactory* factory, const char* owner)
	: base::ProxyUnknown(nullptr)
  , base::ComObjTracer(factory, CID_RECORD, owner)
	, ElementBase(factory)
{
	m_ele_name   = CONSTRUCT_ELEMENT_NAME("record-");
	m_main_type  = EleMainType::SINK;
	m_sub_type   = EleSubType::MUXER;
	m_media_type = EleMediaType::AUDIO_VIDEO;

	if (ERR_CODE_OK != CreateAudioSinkPin() || ERR_CODE_OK != CreateVideoSinkPin()) {
		m_pins_created = false;
	}

	int result = av_bsf_alloc(av_bsf_get_by_name("h264_mp4toannexb"), &m_video_bsf_ctx);
	if (result < 0) {
		LOG_ERR("av_bsf_alloc video failed, error:{}", result);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
RecordElement::~RecordElement()
{
	LOG_INF("Destruct {}", m_ele_name);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* RecordElement::CreateInstance(base::IComFactory* factory, 
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_RECORD) == 0) {
		return new RecordElement(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* RecordElement::NDQueryInterface(const char* riid)
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
ErrCode RecordElement::CreateAudioSinkPin()
{
	media::com::AudioCaps audio_pin_caps;
	audio_pin_caps.AddCap(media::AudioCodec::OPUS);
	audio_pin_caps.AddCap(media::AudioChnls::STEREO);
	audio_pin_caps.AddCap(media::AudioChnls::MONO);
	audio_pin_caps.AddCap(media::AudioSBits::S16);
	audio_pin_caps.AddCap(media::AudioSRate::SR_48K);
	audio_pin_caps.AddCap(media::AudioSRate::SR_16K);

	ISinkPin* audio_sink_pin = (ISinkPin*)QI(CID_SINK_PIN, IID_SINK_PIN, m_ele_name);
	if (!audio_sink_pin) {
		LOG_ERR("Create audio sink pin failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != audio_sink_pin->Init(media::MediaType::AUDIO,
			this,
			CONSTRUCT_PIN_NAME("a-sink-pin-"),
			media::util::ToAudioCapsStr(audio_pin_caps),
			this)) {
		LOG_ERR("Init audio sink pin failed!");
		return ERR_CODE_FAILED;
	}

	m_audio_pin_index = static_cast<uint32_t>(m_sink_pins.size());
	m_sink_pins.push_back(audio_sink_pin);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RecordElement::CreateVideoSinkPin()
{
	media::com::VideoCaps video_pin_caps;
	video_pin_caps.AddCap(media::VideoCodec::H264);
	video_pin_caps.AddCap(media::PixelFormat::I420);
	video_pin_caps.AddCap(media::PixelFormat::NV12);
	video_pin_caps.AddCap(media::PixelFormat::YUY2);
	video_pin_caps.AddCap(media::PixelFormat::RGB24);
	video_pin_caps.AddCap(media::VideoRes::RES_1920x1080);
	video_pin_caps.AddCap(media::VideoRes::RES_1280x720);
	video_pin_caps.AddCap(media::VideoRes::RES_640x480);
	video_pin_caps.AddCap(media::VideoRes::RES_640x360);

	ISinkPin* video_sink_pin = (ISinkPin*)QI(CID_SINK_PIN, IID_SINK_PIN, m_ele_name);
	if (!video_sink_pin) {
		LOG_ERR("Create video sink pin failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != video_sink_pin->Init(media::MediaType::VIDEO,
			this,
			CONSTRUCT_PIN_NAME("v-sink-pin-"),
			media::util::ToVideoCapsStr(video_pin_caps),
			this)) {
		LOG_ERR("Init video sink pin failed!");
		return ERR_CODE_FAILED;
	}

	m_video_pin_index = static_cast<uint32_t>(m_sink_pins.size());
	m_sink_pins.push_back(video_sink_pin);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RecordElement::InitStats()
{
	m_data_stats.reset(new util::DataStats(m_factory, g_logger, m_ele_name));
	m_data_stats->Start();

	util::StatsParam v_fr_stats("video-frame-rate", util::StatsType::IAVER, 1000);
	m_video_fr_stats = m_data_stats->AddStats(v_fr_stats);

  util::StatsParam v_br_stats("video-bit-rate",  util::StatsType::IAVER, 1000);
	m_video_br_stats = m_data_stats->AddStats(v_br_stats);

  util::StatsParam a_br_stats("audio-bit-rate",  util::StatsType::IAVER, 1000);
	m_audio_br_stats = m_data_stats->AddStats(a_br_stats);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RecordElement::DoInit(com::IProperty* props)
{
	m_logger = g_logger;

	if (!props) {
		LOG_ERR("Invalid properties!");
		return ERR_CODE_INVALID_PARAM;
	}

	LOG_INF("DoInit, props:{}", props->Dump());

	const char* record_file = props->GetStrValue("record-file");
	if (!record_file) {
		LOG_INF("Cannot find 'record-file' property!");
		return ERR_CODE_INVALID_PARAM;
	}

	m_record_file.assign(record_file);

	int result = avformat_alloc_output_context2(
		&m_output_fmt_ctx,
		NULL,
		"mp4",
		m_record_file.c_str());
	if (result < 0 || !m_output_fmt_ctx) {
		LOG_ERR("avformat_alloc_output_context2 failed, result:{}", result);
		return ERR_CODE_FAILED;
	}	

  InitStats();

	//av_log_set_callback(LogCallback);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RecordElement::DoStart()
{
	LOG_INF("{}", __FUNCTION__);

	//if (m_audio_sink_pin->Cap().empty() || m_sink_pins[m_video_pin_index]->Cap().empty()) {
	if (m_sink_pins[m_video_pin_index]->Cap().empty()) {
		LOG_WRN("Not negotiated!");
		return ERR_CODE_OK;
	}

	// 打开文件IO上下文
	int result = avio_open(&m_output_fmt_ctx->pb, m_record_file.c_str(),
		AVIO_FLAG_WRITE);
	if (result < 0) {
		LOG_ERR("avio_open failed, result:{}", result);
		avformat_free_context(m_output_fmt_ctx);
		return ERR_CODE_FAILED;
	}

	// 写入文件头信息
	result = avformat_write_header(m_output_fmt_ctx, NULL);
	if (result < 0) {
		LOG_ERR("avformat_write_header failed, result:{}", result);
		avformat_free_context(m_output_fmt_ctx);
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RecordElement::DoPause()
{
	LOG_INF("{}", __FUNCTION__);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RecordElement::DoResume()
{
	LOG_INF("{}", __FUNCTION__);

	//if (m_audio_sink_pin->Cap().empty() || m_sink_pins[m_video_pin_index]->Cap().empty()) {
	if (m_sink_pins[m_video_pin_index]->Cap().empty()) {
		LOG_WRN("Not negotiated!");
		return ERR_CODE_OK;
	}

	// 打开文件IO上下文
	int result = avio_open(&m_output_fmt_ctx->pb, m_record_file.c_str(),
		AVIO_FLAG_WRITE);
	if (result < 0) {
		LOG_ERR("avio_open failed, result:{}", result);
		avformat_free_context(m_output_fmt_ctx);
		return ERR_CODE_FAILED;
	}

	// 写入文件头信息
	result = avformat_write_header(m_output_fmt_ctx, NULL);
	if (result < 0) {
		LOG_ERR("avformat_write_header failed, result:{}", result);
		avformat_free_context(m_output_fmt_ctx);
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RecordElement::DoStop()
{
	LOG_INF("{}", __FUNCTION__);

	if (m_output_fmt_ctx) {
		av_write_trailer(m_output_fmt_ctx);
		avio_close(m_output_fmt_ctx->pb);
		avformat_free_context(m_output_fmt_ctx);
		m_output_fmt_ctx = nullptr;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// TODO: Repeat
//------------------------------------------------------------------------------
ErrCode RecordElement::OpenVideoStream()
{
	AVCodec* codec = avcodec_find_encoder(media::util::ToFfVideoCodec(
		m_video_sink_pin_cap.codec));
	if (!codec) {
		LOG_ERR("Fail to find video encoder:{}!", m_video_sink_pin_cap.codec);
		return ERR_CODE_FAILED;
	}

	AVCodecContext* m_video_codec_ctx = avcodec_alloc_context3(codec);
	m_video_codec_ctx->codec_id = media::util::ToFfVideoCodec(m_video_sink_pin_cap.codec);
	m_video_codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
	m_video_codec_ctx->pix_fmt = media::util::ToFfPixelFormat(m_video_sink_pin_cap.format);
	m_video_codec_ctx->width = media::util::GetWidth(m_video_sink_pin_cap.res);
	m_video_codec_ctx->height = media::util::GetHeight(m_video_sink_pin_cap.res);
	m_video_codec_ctx->bit_rate = 8000000;
	m_video_codec_ctx->bit_rate_tolerance = 4000000;
	m_video_codec_ctx->gop_size = 30;
	m_video_codec_ctx->max_b_frames = 0;
	m_video_codec_ctx->time_base.num = 1;
	m_video_codec_ctx->time_base.den = 50; // 15fps

	/* Some container formats (like MP4) require global headers to be present
	   Mark the encoder so that it behaves accordingly. */
	if (m_output_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
		m_video_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	int result = avcodec_open2(m_video_codec_ctx, codec, NULL);
	if (result != 0) {
		LOG_ERR("avcodec_open2 failed, error:{}!", result);
		return ERR_CODE_FAILED;
	}

	// 创建写入的视频流
	m_video_stream = avformat_new_stream(m_output_fmt_ctx, codec);
	if (!m_video_stream) {
		LOG_ERR("Fail to create video stream!");
		return ERR_CODE_FAILED;
	}

	result = avcodec_parameters_from_context(m_video_stream->codecpar, m_video_codec_ctx);
	if (result < 0) {
		LOG_ERR("avcodec_parameters_from_context failed, error:{}", result);
		return ERR_CODE_FAILED;
	}

	AVCodecParameters* p = avcodec_parameters_alloc();
	result = avcodec_parameters_copy(p, m_video_stream->codecpar);
	if (result < 0) {
		LOG_ERR("avcodec_parameters_copy failed, error:{}", result);
		return ERR_CODE_FAILED;
	}
	m_video_bsf_ctx->par_in = p;
	result = av_bsf_init(m_video_bsf_ctx);
	if (result < 0) {
		LOG_ERR("av_bsf_init failed, error:{}", result);
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// TODO: Repeat
//------------------------------------------------------------------------------
ErrCode RecordElement::OpenAudioStream()
{
	AVCodec* codec = avcodec_find_encoder(media::util::ToFfAudioCodec(
		m_audio_sink_pin_cap.codec));
	if (!codec) {
		LOG_ERR("Fail to find audio encoder!");
		return ERR_CODE_FAILED;
	}
	AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
	codec_ctx->codec_id = media::util::ToFfAudioCodec(m_audio_sink_pin_cap.codec);
	codec_ctx->codec_type = AVMEDIA_TYPE_AUDIO;
	codec_ctx->sample_fmt = media::util::ToFfSampleFormat(m_audio_sink_pin_cap.sbits);
	codec_ctx->sample_rate = media::util::GetSRateNum(m_audio_sink_pin_cap.srate);
	codec_ctx->channels = media::util::GetChnlsNum(m_audio_sink_pin_cap.chnls);

	int result = avcodec_open2(codec_ctx, codec, NULL);
	if (result != 0) {
		LOG_ERR("avcodec_open2 failed, error:{}!", result);
		return ERR_CODE_FAILED;
	}

	// 创建写入的视频流
	m_audio_stream = avformat_new_stream(m_output_fmt_ctx, codec);
	if (!m_audio_stream) {
		LOG_ERR("Fail to create audio stream!");
		return ERR_CODE_FAILED;
	}

	result = avcodec_parameters_from_context(m_audio_stream->codecpar, codec_ctx);
	if (result < 0) {
		LOG_ERR("avcodec_parameters_from_context failed, error:{}", result);
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RecordElement::OnAudioPinData(ISinkPin* pin, const PinData& data)
{
	AVRational src_tb;
	src_tb.num = data.tbn;
	src_tb.den = data.tbd;

	int64_t pts = av_rescale_q_rnd(
		data.pts,
		src_tb,
		m_audio_stream->time_base,
		(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
	int64_t dts = av_rescale_q_rnd(
		data.dts,
		src_tb,
		m_audio_stream->time_base,
		(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
	int64_t duration = av_rescale_q(data.drt, src_tb, m_audio_stream->time_base);

	LOG_INF("rescale dts:{}, pts:{}, drt:{}", dts, pts, duration);

	AVPacket* packet = av_packet_alloc();
	packet->dts = dts;
	packet->pts = pts;
	packet->duration = duration;
	packet->data = data.media_data->data.get();
	packet->size = data.media_data->data_len;
	packet->pos = data.pos;
	packet->stream_index = m_audio_stream->index;

	int result = av_interleaved_write_frame(m_output_fmt_ctx, packet);
	if (result != 0) {
		LOG_ERR("av_interleaved_write_frame failed, error:{}", result);
		av_packet_unref(packet);
		return ERR_CODE_FAILED;
	}

	m_data_stats->OnData(m_audio_br_stats, data.media_data->data_len);

	av_packet_unref(packet);
	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RecordElement::OnVideoPinData(ISinkPin* pin, const PinData& data)
{
	AVRational src_tb;
	src_tb.num = data.tbn;
	src_tb.den = data.tbd;

	int64_t pts = av_rescale_q_rnd(
		data.pts,
		src_tb,
		m_video_stream->time_base,
		(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
	int64_t dts = av_rescale_q_rnd(
		data.dts,
		src_tb,
		m_video_stream->time_base,
		(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
	int64_t duration = av_rescale_q(data.drt, src_tb, m_video_stream->time_base);

	LOG_INF("rescale dts:{}, pts:{}, drt:{}", dts, pts, duration);

	AVPacket* packet = av_packet_alloc();
	packet->dts = dts;
	packet->pts = pts;
	packet->duration = duration;
	packet->data = data.media_data->data.get();
	packet->size = data.media_data->data_len;
	packet->pos = -1;
	packet->stream_index = m_video_stream->index;

	int result = av_bsf_send_packet(m_video_bsf_ctx, packet);
	if (result == 0) {
		result = av_bsf_receive_packet(m_video_bsf_ctx, packet);
		if (result != 0) {
			LOG_ERR("av_bsf_receive_packet failed, result:{}", result);
			av_packet_unref(packet);
			return ERR_CODE_FAILED;
		}
	}
	else {
		LOG_ERR("av_bsf_send_packet failed, result:{}", result);
		av_packet_unref(packet);
		return ERR_CODE_FAILED;
	}

	result = av_interleaved_write_frame(m_output_fmt_ctx, packet);
	if (result != 0) {
		LOG_ERR("av_interleaved_write_frame failed, error:{}", result);
		av_packet_unref(packet);
		return ERR_CODE_FAILED;
	}

	m_data_stats->OnData(m_video_fr_stats, 1);
	m_data_stats->OnData(m_video_br_stats, data.media_data->data_len);

	av_packet_unref(packet);
	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RecordElement::OnSinkPinData(ISinkPin* pin, const PinData& data)
{
	if (m_ele_state != EleState::RUNNING) {
		LOG_ERR("Invalid element state:{}", m_ele_state);
		return ERR_CODE_FAILED;
	}

	if (!m_output_fmt_ctx) {
		LOG_ERR("Invalid avformat context!");
		return ERR_CODE_FAILED;
	}

	if (data.mt == media::MediaType::AUDIO) {
		return OnAudioPinData(pin, data);
	}
	else {
		return OnVideoPinData(pin, data);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RecordElement::OnSinkPinNegotiated(ISinkPin* pin, const std::string& cap)
{
	LOG_INF("Sink pin negotiated, pin:{}, cap:{}", pin->ToStr(),
		media::util::Capper(cap));

	if (pin->Name() == m_sink_pins[m_audio_pin_index]->Name()) {
		auto audio_cap = media::util::ParseAudioCap(cap);
		if (!audio_cap.has_value()) {
			m_audio_sink_pin_cap = audio_cap.value();
		}
		else {
			LOG_ERR("Parse audio cap failed, cap:{}", cap);
			return ERR_CODE_FAILED;
		}
	}

	if (pin->Name() == m_sink_pins[m_video_pin_index]->Name()) {
		auto video_cap = media::util::ParseVideoCap(cap);
		if (!video_cap.has_value()) {
			m_video_sink_pin_cap = video_cap.value();
		}
		else {
			LOG_ERR("Parse video cap failed, cap:{}", cap);
			return ERR_CODE_FAILED;
		}
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RecordElement::OnPinEndOfStream(ISinkPin* pin, const PinMsg& msg)
{
	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode RecordElement::ProcSinkPinMsg(ISinkPin* pin, const PinMsg& msg)
{
	switch (msg.msg_type) {
	case PinMsgType::END_STREAM:
		return OnPinEndOfStream(pin, msg);
	default:
		return ERR_CODE_MSG_NO_PROC;
	}
}

}