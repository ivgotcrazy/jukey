#pragma once

#include "component.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "if-element.h"
#include "if-pin.h"
#include "if-pipeline.h"
#include "element-base.h"
#include "util-streamer.h"
#include "stream-dumper.h"
#include "common-config.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avstring.h>
#include <libavutil/avutil.h>
}

#include "opus.h"

// yaml-cpp warning
#pragma warning( disable: 4251 )
#pragma warning( disable: 4275 )


namespace jukey::stmr
{

//==============================================================================
// Decode audio sample
//==============================================================================
class AudioDecodeElement
	: public media::util::ElementBase
	, public base::ProxyUnknown
	, public base::ComObjTracer
{
public:
	AudioDecodeElement(base::IComFactory* outer, const char* owner);
	~AudioDecodeElement();

	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// ElementBase
	virtual com::ErrCode DoInit(com::IProperty* props) override;
	virtual com::ErrCode DoStart() override;
	virtual com::ErrCode DoPause() override;
	virtual com::ErrCode DoResume() override;
	virtual com::ErrCode DoStop() override;

	// ISinkPinHandler
	virtual com::ErrCode OnSinkPinData(ISinkPin* pin, 
		const PinData& data) override;
	virtual com::ErrCode OnSinkPinNegotiated(ISinkPin* pin,
		const std::string& cap) override;

	// ISrcPinHandler
	virtual com::ErrCode OnSrcPinNegotiated(ISrcPin* pin,
		const std::string& cap) override;

private:
	com::ErrCode CreateSrcPin();
	com::ErrCode CreateSinkPin();
	com::ErrCode CreateDecoder();
	com::ErrCode CreateFfmpegDecoder();
	com::ErrCode CreateOpusDecoder();
	com::ErrCode DoFfmpegDecode(const PinData& data);
	com::ErrCode DoOpusDecode(const PinData& data);
	void UpdatePinCap(const std::string& src_cap, const std::string& sink_cap);
	void DestroyDecoder();
	void ParseElementConfig();

private:
	uint32_t m_src_pin_index = 0;
	uint32_t m_sink_pin_index = 0;

	media::com::AudioCap m_src_pin_cap;
	media::com::AudioCap m_sink_pin_cap;

	void* m_data = nullptr;

	// FFMPEG
	AVCodecContext* m_codec_ctx = nullptr;
	AVCodec* m_codec = nullptr;
	AVFrame* m_frame = nullptr;

	OpusDecoder* m_opus_decoder = nullptr;

	bool m_use_ffmpeg = false; // TODO: something wrong to ffmpeg

	media::util::StreamDumperSP m_bd_dumper; // before decode
	media::util::StreamDumperSP m_ad_dumper; // after decode
};

}