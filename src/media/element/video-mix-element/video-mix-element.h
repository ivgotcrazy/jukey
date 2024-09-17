#pragma once

#include "component.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "element-base.h"
#include "if-pin.h"
#include "if-pipeline.h"
#include "util-streamer.h"


namespace jukey::stmr
{

//==============================================================================
// 
//==============================================================================
class VideoMixElement
	: public media::util::ElementBase
	, public base::ProxyUnknown
	, public base::ComObjTracer
{
public:
	VideoMixElement(base::IComFactory* factory, const char* owner);
	~VideoMixElement();

	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IElement
	virtual com::ErrCode DoInit(com::IProperty* param) override;

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

private:
	uint32_t m_src_pin_index = 0;
	uint32_t m_sink_pin_index = 0;

	media::com::VideoCap m_src_pin_cap;
	media::com::VideoCap m_sink_pin_cap;
};

}