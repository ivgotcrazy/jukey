#pragma once

#include "component.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "element-base.h"
#include "if-pin.h"
#include "if-pipeline.h"
#include "common/util-stats.h"
#include "util-streamer.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
}


namespace jukey::stmr
{

//==============================================================================
// 
//==============================================================================
class RecordElement
	: public media::util::ElementBase
	, public base::ProxyUnknown
	, public base::ComObjTracer
{
public:
	RecordElement(base::IComFactory* factory, const char* owner);
	~RecordElement();

	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IElement
	virtual com::ErrCode DoInit(com::IProperty* param) override;
	virtual com::ErrCode DoStart() override;
	virtual com::ErrCode DoPause() override;
	virtual com::ErrCode DoResume() override;
	virtual com::ErrCode DoStop() override;
	virtual com::ErrCode ProcSinkPinMsg(ISinkPin* pin, 
		const PinMsg& msg) override;

	// ISinkPinHandler
	virtual com::ErrCode OnSinkPinData(ISinkPin* pin, 
		const PinData& data) override;
	virtual com::ErrCode OnSinkPinNegotiated(ISinkPin* pin,
		const std::string& cap) override;

private:
	com::ErrCode CreateAudioSinkPin();
	com::ErrCode CreateVideoSinkPin();
	com::ErrCode OpenAudioStream();
	com::ErrCode OpenVideoStream();
	com::ErrCode OnPinEndOfStream(ISinkPin* pin, const PinMsg& msg);
	com::ErrCode OnAudioPinData(ISinkPin* pin, const PinData& data);
	com::ErrCode OnVideoPinData(ISinkPin* pin, const PinData& data);
  void InitStats();

private:
	uint32_t m_audio_pin_index = 0;
	uint32_t m_video_pin_index = 0;

	media::com::AudioCap m_audio_sink_pin_cap;
	media::com::VideoCap m_video_sink_pin_cap;

	std::string m_record_file;

	// FFMPEG
	AVFormatContext* m_output_fmt_ctx = nullptr;
	AVStream* m_audio_stream = nullptr;
	AVStream* m_video_stream = nullptr;
	AVCodecContext* m_audio_codec_ctx = nullptr;
	AVCodecContext* m_video_codec_ctx = nullptr;

	std::shared_ptr<util::DataStats> m_data_stats;
	util::StatsId m_audio_br_stats = INVALID_STATS_ID;
	util::StatsId m_video_br_stats = INVALID_STATS_ID;
	util::StatsId m_video_fr_stats = INVALID_STATS_ID;

	AVBSFContext* m_video_bsf_ctx = nullptr;
};

}