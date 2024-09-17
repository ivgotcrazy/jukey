#pragma once

#include "component.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "if-element.h"
#include "if-pin.h"
#include "if-pipeline.h"
#include "thread/common-thread.h"
#include "element-base.h"
#include "common/util-stats.h"
#include "util-streamer.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
};

namespace jukey::stmr
{

//==============================================================================
// 
//==============================================================================
class FileDemuxElement
	: public media::util::ElementBase
	, public base::ProxyUnknown
	, public base::ComObjTracer
	, public util::CommonThread
{
public:
	FileDemuxElement(base::IComFactory* factory, const char* owner);
	~FileDemuxElement();

	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IElement
	virtual com::ErrCode DoInit(com::IProperty* props) override;
	virtual com::ErrCode DoStart() override;
	virtual com::ErrCode DoPause() override;
	virtual com::ErrCode DoResume() override;
	virtual com::ErrCode DoStop() override;

	// ISrcPinHandler
	virtual com::ErrCode OnSrcPinNegotiated(ISrcPin* pin,
		const std::string& cap) override;

	// CommonThread
	virtual void ThreadProc() override;

	// ElementBase
	virtual com::ErrCode PreProcPipelineMsg(const com::CommonMsg& msg) override;

private:
	virtual com::ErrCode ProcSrcPinMsg(ISrcPin* pin, const PinMsg& msg);

	com::ErrCode OnAudioStream(uint32_t stream_index, AVCodecParameters* par);
	com::ErrCode OnVideoStream(uint32_t stream_index, AVCodecParameters* par);
	com::ErrCode ParseAudioStream(uint32_t stream_index, AVCodecParameters* par);
	com::ErrCode ParseVideoStream(uint32_t stream_index, AVCodecParameters* par);

	bool OpenMediaFile(CSTREF file_path);

	bool UpdateAudioAvaiCaps(AVCodecParameters* par);
	bool UpdateVideoAvaiCaps(AVCodecParameters* par);

	com::ErrCode CreateAudioSrcPin();
	com::ErrCode CreateVideoSrcPin();

	void ProcAudioPacket(AVPacket* packet);
	void ProcVideoPacket(AVPacket* packet);

	void NotifyElementStream();
	void NotifyPlayProgress();
	void NotifyFlush();

	com::ErrCode OnSeekBeginMsg(const com::CommonMsg& msg);
	com::ErrCode OnSeekEndMsg(const com::CommonMsg& msg);

private:
	uint32_t m_audio_pin_index = 0;
	uint32_t m_video_pin_index = 0;

	std::string m_media_file;

	// FFMPEG
	AVFormatContext* m_avfmt_ctx = nullptr;
	int m_video_index = -1;
	int m_audio_index = -1;
	AVRational m_audio_time_base = {0};
	AVRational m_video_time_base = {0};

	// AnnexB
	AVBSFContext* m_video_bsf_ctx = nullptr;

	// Statistics
	util::DataStatsSP m_data_stats;
	util::StatsId m_apr_stats = INVALID_STATS_ID;
	util::StatsId m_vpr_stats = INVALID_STATS_ID;

	// Control video frame rate
	uint32_t m_frame_rate = 0;

	uint64_t m_duration = 0; // in micro second
	uint64_t m_cur_time = 0; // in milli second

	bool m_seeking = false;
	int32_t m_progress = -1;
	uint64_t m_last_notify_progress = 0;

	std::string m_audio_stream_id;
	std::string m_video_stream_id;
};

}