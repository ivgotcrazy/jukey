#pragma once

#include "component.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "element-base.h"
#include "if-pin.h"
#include "if-pipeline.h"
#include "util-streamer.h"
#include "stream-dumper.h"

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
class VideoDecodeElement
	: public media::util::ElementBase
	, public base::ProxyUnknown
	, public base::ComObjTracer
{
public:
	VideoDecodeElement(base::IComFactory* factory, const char* owner);
	~VideoDecodeElement();

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
	com::ErrCode TryCreateDecoder();
	void UpdatePinCap(const std::string& src_cap, const std::string& sink_cap);
	void DestroyDecoder();
	void ParseElementConfig();

private:
	uint32_t m_src_pin_index = 0;
	uint32_t m_sink_pin_index = 0;

	media::com::VideoCap m_src_pin_cap;
	media::com::VideoCap m_sink_pin_cap;

	AVCodecContext* m_codec_ctx = nullptr;
	AVCodec* m_codec = nullptr;
	AVFrame* m_frame = nullptr;

	media::util::StreamDumperSP m_bd_dumper; // before decode
	media::util::StreamDumperSP m_ad_dumper; // after decode
};

}