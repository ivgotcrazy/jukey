#pragma once

#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "util-streamer.h"
#include "if-pin.h"
#include "element-base.h"
#include "log.h"

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


namespace jukey::stmr
{

//==============================================================================
// 
//==============================================================================
class VideoConvertElement 
	: public base::ProxyUnknown
	, public base::ComObjTracer
	, public media::util::ElementBase
{
public:
	VideoConvertElement(base::IComFactory* factory, const char* owner);
	~VideoConvertElement();

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
	virtual com::ErrCode OnSrcPinNegotiated(ISrcPin* src_pin,
		const std::string& cap) override;

private:
	com::ErrCode CreateSrcPin();
	com::ErrCode CreateSinkPin();
	com::ErrCode CreateSwsContext();
	com::ErrCode ConvertPinData(const PinData& data);
	com::ErrCode SetSrcDataAndLineSize(const PinData& data,
		uint8_t** src_data, int* linesize);
	void UpdatePinCap(const std::string& src_cap,
		const std::string& sink_cap);
	void DestroySwsContext();
	void TryCreateSwsContext();

private:
	uint32_t m_src_pin_index = 0;
	uint32_t m_sink_pin_index = 0;

	media::com::VideoCap m_src_pin_cap;
	media::com::VideoCap m_sink_pin_cap;

	SwsContext* m_sws_ctx = nullptr;
	bool m_need_convert = true;
	bool m_use_planar = true;
};

}