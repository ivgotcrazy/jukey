#include "file-demux-element.h"
#include "pipeline-msg.h"
#include "nlohmann/json.hpp"
#include "util-streamer.h"
#include "common/util-time.h"
#include "common/util-common.h"
#include "util-ffmpeg.h"
#include "util-streamer.h"
#include "log.h"
#include "common/media-common-define.h"
#include "common/media-common-struct.h"

using namespace jukey::util;
using namespace jukey::com;

#define AUDIO_TIME_BASE \
	(av_q2d(m_avfmt_ctx->streams[m_audio_index]->time_base))

#define VIDEO_TIME_BASE \
	(av_q2d(m_avfmt_ctx->streams[m_video_index]->time_base))


namespace
{

//------------------------------------------------------------------------------
// Media ID: this + index
//------------------------------------------------------------------------------
std::string MakeMediaId(void* ptr, uint32_t index)
{
	std::string media_id;
	media_id.append(std::to_string((intptr_t)ptr)).append(std::to_string(index));
	return media_id;
}

}


namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
FileDemuxElement::FileDemuxElement(base::IComFactory* factory, const char* owner)
	: base::ProxyUnknown(nullptr)
  , base::ComObjTracer(factory, CID_FILE_DEMUX, owner)
	, ElementBase(factory)
	, util::CommonThread(m_ele_name, true)
{
	m_ele_name   = CONSTRUCT_ELEMENT_NAME("file-demux-");
	m_main_type  = EleMainType::SRC;
	m_sub_type   = EleSubType::DEMUXER;
	m_media_type = EleMediaType::AUDIO_VIDEO;

	if (ERR_CODE_OK != CreateAudioSrcPin() || ERR_CODE_OK != CreateVideoSrcPin()) {
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
FileDemuxElement::~FileDemuxElement()
{
	LOG_INF("Destruct {}", m_ele_name);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* FileDemuxElement::CreateInstance(base::IComFactory* factory, 
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_FILE_DEMUX) == 0) {
		return new FileDemuxElement(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* FileDemuxElement::NDQueryInterface(const char* riid)
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
ErrCode FileDemuxElement::CreateAudioSrcPin()
{
	media::com::AudioCaps audio_pin_caps;
	audio_pin_caps.AddCap(media::AudioCodec::OPUS);
	audio_pin_caps.AddCap(media::AudioCodec::AAC);
	audio_pin_caps.AddCap(media::AudioChnls::MONO);
	audio_pin_caps.AddCap(media::AudioChnls::STEREO);
	audio_pin_caps.AddCap(media::AudioSBits::S16);
	audio_pin_caps.AddCap(media::AudioSBits::S32);
	audio_pin_caps.AddCap(media::AudioSBits::S16P);
	audio_pin_caps.AddCap(media::AudioSBits::S32P);
	audio_pin_caps.AddCap(media::AudioSBits::FLTP);
	audio_pin_caps.AddCap(media::AudioSRate::SR_16K);
	audio_pin_caps.AddCap(media::AudioSRate::SR_48K);

	ISrcPin* audio_src_pin = (ISrcPin*)QI(CID_SRC_PIN, IID_SRC_PIN, m_ele_name);
	if (!audio_src_pin) {
		LOG_ERR("Create audio src pin failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != audio_src_pin->Init(media::MediaType::AUDIO,
			this,
			CONSTRUCT_PIN_NAME("a-src-pin-"),
			media::util::ToAudioCapsStr(audio_pin_caps),
			this)) {
		LOG_ERR("Init audio src pin failed!");
		return ERR_CODE_FAILED;
	}

	m_audio_pin_index = static_cast<uint32_t>(m_src_pins.size());
	m_src_pins.push_back(audio_src_pin);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode FileDemuxElement::CreateVideoSrcPin()
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

	ISrcPin* video_src_pin = (ISrcPin*)QI(CID_SRC_PIN, IID_SRC_PIN, m_ele_name);
	if (!video_src_pin) {
		LOG_ERR("Create src pin failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != video_src_pin->Init(media::MediaType::VIDEO,
			this,
			CONSTRUCT_PIN_NAME("v-src-pin-"),
			media::util::ToVideoCapsStr(video_pin_caps),
			this)) {
		LOG_ERR("Init video src pin failed!");
		return ERR_CODE_FAILED;
	}

	m_video_pin_index = static_cast<uint32_t>(m_src_pins.size());
	m_src_pins.push_back(video_src_pin);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode FileDemuxElement::DoInit(com::IProperty* props)
{
	m_logger = g_logger;

	if (!props) {
		LOG_ERR("Invalid properties!");
		return ERR_CODE_INVALID_PARAM;
	}

	LOG_INF("DoInit, props:{}", props->Dump());

	// Media file path
	const char* file_path = props->GetStrValue("file-path");
	if (!file_path) {
		LOG_ERR("Cannot find 'file-path' from properties!");
		return ERR_CODE_INVALID_PARAM;
	}
	m_media_file.assign(file_path);

	if (!OpenMediaFile(m_media_file)) {
		LOG_ERR("Open media file:{} failed!", m_media_file);
		return ERR_CODE_FAILED;
	}

	// Seek control
	m_pipeline->SubscribeMsg(PlMsgType::PL_SEEK_BEGIN, this);
	m_pipeline->SubscribeMsg(PlMsgType::PL_SEEK_END, this);

	m_data_stats.reset(new DataStats(m_factory, g_logger, m_ele_name));
	m_data_stats->Start();

	StatsParam apr_stats("audio-packet-rate", StatsType::IAVER, 1000);
	m_apr_stats = m_data_stats->AddStats(apr_stats);

	StatsParam vpr_stats("video-packet-rate", StatsType::IAVER, 1000);
	m_vpr_stats = m_data_stats->AddStats(vpr_stats);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// Stream ID generated before open media file 
//------------------------------------------------------------------------------
void FileDemuxElement::NotifyElementStream()
{
	if (!m_audio_stream_id.empty()) {
		ElementStream* es = new ElementStream();
		es->stream.src.src_type = MediaSrcType::FILE;
		es->stream.src.src_id = m_media_file;
		es->stream.stream.stream_type = StreamType::AUDIO;
		es->stream.stream.stream_id = m_audio_stream_id;
		es->pin.ele_name = m_ele_name;
		es->pin.pin_name = m_src_pins[m_audio_pin_index]->Name();
		es->cap = m_src_pins[m_audio_pin_index]->Cap();

		com::CommonMsg msg;
		msg.msg_type = (uint32_t)PlMsgType::ADD_ELEMENT_STREAM;
		msg.src = m_ele_name;
		msg.msg_data.reset(es);
		m_pipeline->PostPlMsg(msg);
	}

	if (!m_video_stream_id.empty()) {
		ElementStream* es = new ElementStream();
		es->stream.src.src_type = MediaSrcType::FILE;
		es->stream.src.src_id = m_media_file;
		es->stream.stream.stream_type = StreamType::VIDEO;
		es->stream.stream.stream_id = m_video_stream_id;
		es->pin.ele_name = m_ele_name;
		es->pin.pin_name = m_src_pins[m_video_pin_index]->Name();
		es->cap = m_src_pins[m_video_pin_index]->Cap();

		com::CommonMsg msg;
		msg.msg_type = (uint32_t)PlMsgType::ADD_ELEMENT_STREAM;
		msg.src = m_ele_name;
		msg.msg_data.reset(es);
		m_pipeline->PostPlMsg(msg);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode FileDemuxElement::DoStart()
{
	StartThread();

	NotifyElementStream();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode FileDemuxElement::DoPause()
{
	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode FileDemuxElement::DoResume()
{
	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode FileDemuxElement::DoStop()
{
	StopThread();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool FileDemuxElement::UpdateAudioAvaiCaps(AVCodecParameters* par)
{
	media::com::AudioCap acap;

	acap.codec = media::util::ToAudioCodecFF(par->codec_id);
	if (acap.codec == media::AudioCodec::INVALID) {
		LOG_ERR("Invalid audio codec:{}", par->codec_id);
		return false;
	}

	acap.chnls = media::util::ToAudioChnlsFF(par->channels);
	if (acap.chnls == media::AudioChnls::INVALID) {
		LOG_ERR("Invalid audio channels:{}", par->channels);
		return false;
	}

	acap.srate = media::util::ToAudioSRateFF(par->sample_rate);
	if (acap.srate == media::AudioSRate::INVALID) {
		LOG_ERR("Invalid audio sample rate:{}", par->sample_rate);
		return false;
	}

	acap.sbits = media::util::ToAudioSBitsFF(par->format);
	if (acap.sbits == media::AudioSBits::INVALID) {
		LOG_ERR("Invalid audio sample bits:{}", par->format);
		return false;
	}

	PinCaps avai_caps;
	avai_caps.push_back(media::util::ToAudioCapStr(acap));

	LOG_INF("Update available caps:");

	if (ERR_CODE_OK != m_src_pins[m_audio_pin_index]->UpdateAvaiCaps(avai_caps)) {
		LOG_ERR("Update src pin available caps failed");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode
FileDemuxElement::OnAudioStream(uint32_t stream_index, AVCodecParameters* par)
{
	LOG_INF("OnAudioStream, stream_index:{}", stream_index);

	m_audio_stream_id = util::GenerateGUID();

	if (ERR_CODE_OK != m_src_pins[m_audio_pin_index]->OnPinMsg(nullptr,
		PinMsg(PinMsgType::SET_STREAM, m_audio_stream_id))) {
		LOG_ERR("Set audio src pin stream failed!");
		return ERR_CODE_FAILED;
	}

	if (!UpdateAudioAvaiCaps(par)) {
		LOG_ERR("Update audio available caps failed");
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode
FileDemuxElement::ParseAudioStream(uint32_t stream_index, AVCodecParameters* par)
{
	m_audio_index = stream_index;

	m_audio_time_base = m_avfmt_ctx->streams[stream_index]->time_base;

	return OnAudioStream(stream_index, par);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool FileDemuxElement::UpdateVideoAvaiCaps(AVCodecParameters* par)
{
	media::com::VideoCap vcap;

	vcap.codec = media::util::ToVideoCodecFF(par->codec_id);
	if (vcap.codec == media::VideoCodec::INVALID) {
		LOG_ERR("Invalid video codec:{}", par->codec_id);
		return false;
	}

	vcap.res = media::util::ToVideoRes(par->width, par->height);
	if (vcap.res == media::VideoRes::INVALID) {
		LOG_ERR("Invalid video resolution:{}|{}", par->width, par->height);
		return false;
	}

	vcap.format = media::util::ToPixelFormatFF((AVPixelFormat)par->format);
	if (vcap.format == media::PixelFormat::INVALID) {
		LOG_ERR("Invalid pixel format:{}", par->format);
		return false;
	}

	PinCaps avai_caps;
	avai_caps.push_back(media::util::ToVideoCapStr(vcap));

	if (ERR_CODE_OK != m_src_pins[m_video_pin_index]->UpdateAvaiCaps(avai_caps)) {
		LOG_ERR("Update src pin available caps failed");
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
// MP4->Annex
//------------------------------------------------------------------------------
ErrCode FileDemuxElement::OnVideoStream(uint32_t stream_index,
	AVCodecParameters* par)
{
	LOG_INF("OnVideoStream, stream_index:{}", stream_index);

	AVCodecParameters* p = avcodec_parameters_alloc();
	int result = avcodec_parameters_copy(p, par);
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

	m_video_stream_id = util::GenerateGUID();

	if (ERR_CODE_OK != m_src_pins[m_video_pin_index]->OnPinMsg(nullptr,
		PinMsg(PinMsgType::SET_STREAM, m_video_stream_id))) {
		LOG_ERR("Set video src pin stream failed!");
		return ERR_CODE_FAILED;
	}

	if (!UpdateVideoAvaiCaps(par)) {
		LOG_ERR("Update video available caps failed");
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// MP4->Annex
//------------------------------------------------------------------------------
ErrCode FileDemuxElement::ParseVideoStream(uint32_t stream_index,
	AVCodecParameters* par)
{
	m_video_index = stream_index;
	
	m_video_time_base = m_avfmt_ctx->streams[stream_index]->time_base;

	AVRational frame_rate = m_avfmt_ctx->streams[m_video_index]->r_frame_rate;
	m_frame_rate = (uint32_t)av_q2d(frame_rate);

	return OnVideoStream(stream_index, par);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool FileDemuxElement::OpenMediaFile(CSTREF file_path)
{
	LOG_INF("Open media file:{}", file_path);

	int result = avformat_open_input(&m_avfmt_ctx, file_path.c_str(), 0, 0);
	if (result != 0) {
		LOG_ERR("Open file:{} failed, ret:{}", file_path, result);
		return false;
	}

	m_avfmt_ctx->avoid_negative_ts = AVFMT_AVOID_NEG_TS_MAKE_NON_NEGATIVE;

	if (avformat_find_stream_info(m_avfmt_ctx, NULL) < 0) {
		LOG_ERR("Couldn't find stream information");
		return false;
	}

	m_duration = m_avfmt_ctx->duration;

	for (unsigned int i = 0; i < m_avfmt_ctx->nb_streams; i++) {
		AVCodecParameters* params = m_avfmt_ctx->streams[i]->codecpar;
		if (params->codec_type == AVMEDIA_TYPE_VIDEO) {
			if (ERR_CODE_OK != ParseVideoStream(i, params)) {
				return false;
			}
		}
		else if (params->codec_type == AVMEDIA_TYPE_AUDIO) {
			if (ERR_CODE_OK != ParseAudioStream(i, params)) {
				return false;
			}
		}
	}

	if (m_audio_index == -1 && m_video_index == -1) {
		LOG_ERR("Cannot find any audio or video stream");
		return false;
	}

	LOG_INF("Init success, audio index:{}, video index:{}", m_audio_index,
		m_video_index);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FileDemuxElement::ProcAudioPacket(AVPacket* packet)
{
	m_data_stats->OnData(m_apr_stats, 1);

	if (m_ele_state != EleState::RUNNING) {
		return;
	}

	PinData audio_data(media::MediaType::AUDIO, packet->data, packet->size);
	audio_data.dts = packet->dts;
	audio_data.pts = packet->pts;
	audio_data.pos = packet->pos;
	audio_data.drt = packet->duration;
	audio_data.syn = (uint64_t)this;
	audio_data.tbn = m_audio_time_base.num;
	audio_data.tbd = m_audio_time_base.den;

	m_src_pins[m_audio_pin_index]->OnPinData(audio_data);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FileDemuxElement::ProcVideoPacket(AVPacket* packet)
{
	if (m_ele_state != EleState::RUNNING) {
		return;
	}

	int result = av_bsf_send_packet(m_video_bsf_ctx, packet);
	if (0 == result) {
		result = av_bsf_receive_packet(m_video_bsf_ctx, packet);
		if (0 != result) {
			LOG_ERR("av_bsf_receive_packet failed, result:{}", result);
			return;
		}
	}
	else {
		LOG_ERR("av_bsf_send_packet failed, result:{}", result);
		return;
	}

	m_data_stats->OnData(m_vpr_stats, 1);

	// TODO: PTS is not monotonically increasing
	if (packet->dts >= 0) {
		m_cur_time = (uint64_t)(packet->dts * av_q2d(m_video_time_base) * 1000000);
	}

	LOG_DBG("pts:{}, dts:{}, time:{}", packet->pts, packet->dts, m_cur_time);

	PinData video_data(media::MediaType::VIDEO);
	video_data.dts = packet->dts;
	video_data.pts = packet->pts;
	video_data.pos = packet->pos;
	video_data.drt = packet->duration;
	video_data.syn = (uint64_t)this;
	video_data.tbn = m_video_time_base.num;
	video_data.tbd = m_video_time_base.den;

	// Avoid data copy
	video_data.media_data[0].data.reset(packet->data, util::NoDestruct);
	video_data.media_data[0].data_len = packet->size;
	video_data.media_data[0].total_len = packet->size;
	video_data.data_count = 1;

	m_src_pins[m_video_pin_index]->OnPinData(video_data);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FileDemuxElement::NotifyPlayProgress()
{
	if (m_seeking) return;

	uint32_t progress = (uint32_t)(m_cur_time * 1000 / m_duration);

	com::MediaSrc msrc(MediaSrcType::FILE, m_media_file);

	com::CommonMsg msg;
	msg.msg_type = (uint32_t)PlMsgType::PLAY_PROGRESS;
	msg.src = m_ele_name;
	msg.msg_data.reset(new PlayProgressData(msrc, progress));

	m_pipeline->PostPlMsg(msg);

	m_last_notify_progress = progress;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FileDemuxElement::NotifyFlush()
{
	m_src_pins[m_video_pin_index]->OnPinMsg(nullptr, 
		PinMsg(PinMsgType::FLUSH_DATA));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FileDemuxElement::ThreadProc()
{
	int result = 0;
	AVPacket* packet = av_packet_alloc();

	uint64_t last_notify_time = util::Now();

	while (!m_stop) {
		if (m_ele_state != EleState::RUNNING
			|| !m_avfmt_ctx
			|| (m_audio_index < 0 && m_video_index < 0)
			|| !m_src_pins[m_audio_pin_index]->Negotiated()
			|| !m_src_pins[m_video_pin_index]->Negotiated()) {
			util::Sleep(10);
			continue;
		}

		if (m_seeking && m_progress >= 0) {
			int64_t pos_ts = m_duration * m_progress / 1000;
			constexpr AVRational av_time_base_q = { 1, 1000000 };
			int64_t seek_ts = av_rescale_q(pos_ts, av_time_base_q, m_video_time_base);
			if (av_seek_frame(m_avfmt_ctx, m_video_index, seek_ts, AVSEEK_FLAG_BACKWARD) < 0) {
				LOG_ERR("av_seek_frame failed");
			}
			m_seeking = false;
			NotifyFlush();
		}

		result = av_read_frame(m_avfmt_ctx, packet);
		if (result >= 0) {
			if (packet->stream_index == m_video_index) {
				ProcVideoPacket(packet);
			}
			else if (packet->stream_index == m_audio_index) {
				ProcAudioPacket(packet);
			}
			else {
				LOG_WRN("Unexpected stream index:{}", packet->stream_index);
			}
			av_packet_unref(packet);
		}
		else if (result == AVERROR_EOF) {
			// TODO: 
			util::Sleep(10);
		}
		else {
			LOG_ERR("av_read_frame failed, result:{}", result);
		}

		if (last_notify_time + 10000 < util::Now()) {
			NotifyPlayProgress();
			last_notify_time = util::Now();
		}
	}

	av_packet_free(&packet);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode FileDemuxElement::ProcSrcPinMsg(ISrcPin* pin, const PinMsg& msg)
{
	LOG_INF("OnSrcPinMsg: {}", msg.msg_type);

	if (msg.msg_type == PinMsgType::NO_PENDING) {
		m_ele_state = stmr::EleState::RUNNING;
		return ERR_CODE_MSG_PROC_OK;
	}
	else if (msg.msg_type == PinMsgType::SET_PENDING) {
		m_ele_state = stmr::EleState::PAUSED;
		return ERR_CODE_MSG_PROC_OK;
	}
	else {
		return ERR_CODE_MSG_NO_PROC;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode FileDemuxElement::OnSrcPinNegotiated(ISrcPin* pin, const std::string& cap)
{
	LOG_INF("Src pin negotiated, cap:{}", media::util::Capper(cap));

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode FileDemuxElement::OnSeekBeginMsg(const com::CommonMsg& msg)
{
	m_seeking = true;
	m_progress = -1;

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode FileDemuxElement::OnSeekEndMsg(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(SeekEndMsgData);

	LOG_INF("Received seek msg, progress:{}", data->progress);

	m_progress = (int32_t)data->progress;

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode FileDemuxElement::PreProcPipelineMsg(const CommonMsg& msg)
{
	if (msg.msg_type == PlMsgType::PL_SEEK_BEGIN) {
		return OnSeekBeginMsg(msg);
	}
	else if (msg.msg_type == PlMsgType::PL_SEEK_END) {
		return OnSeekEndMsg(msg);
	}
	else {
		return ERR_CODE_MSG_NO_PROC;
	}
}

}