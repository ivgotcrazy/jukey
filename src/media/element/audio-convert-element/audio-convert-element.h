#pragma once

#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "if-element.h"
#include "if-pin.h"
#include "util-streamer.h"
#include "element-base.h"
#include "stream-dumper.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libswscale/swscale.h>
#include <libavutil/avstring.h>
#include <libavutil/time.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}


// yaml-cpp warning
#pragma warning( disable: 4251 )
#pragma warning( disable: 4275 )

namespace jukey::stmr
{

//==============================================================================
// Convert audio sample format
//==============================================================================
class AudioConvertElement 
	: public base::ProxyUnknown
	, public base::ComObjTracer
	, public media::util::ElementBase
{
public:
	AudioConvertElement(base::IComFactory* factory, const char* owner);
	~AudioConvertElement();

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
	com::ErrCode CreateSwrContext();
	com::ErrCode ConvertPinData(const PinData& data);
	com::ErrCode TryCreateSwrContext();
	void UpdatePinCap(const std::string& src_cap, const std::string& sink_cap);
	void DestroySwrContext();
	void ParseElementConfig();

private:
	media::com::AudioCap m_src_pin_cap;
	media::com::AudioCap m_sink_pin_cap;

	uint32_t m_src_pin_index = 0;
	uint32_t m_sink_pin_index = 0;

	// FFmpeg
	SwrContext* m_swr_ctx = nullptr;

	bool m_need_convert = true;

	media::util::StreamDumperSP m_bc_dumper; // before convert
	media::util::StreamDumperSP m_ac_dumper; // after convert
};

}