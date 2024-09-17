#pragma once

#include "component.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "element-base.h"
#include "if-pin.h"
#include "if-pipeline.h"
#include "util-streamer.h"
#include "if-timer-mgr.h"


namespace jukey::stmr
{

//==============================================================================
// 
//==============================================================================
class AudioTestElement
	: public media::util::ElementBase
	, public base::ProxyUnknown
	, public base::ComObjTracer
{
public:
	AudioTestElement(base::IComFactory* factory, const char* owner);
	~AudioTestElement();

	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// ElementBase
	virtual com::ErrCode DoInit(com::IProperty* param) override;

	// ISinkPinHandler
	virtual com::ErrCode OnSinkPinData(ISinkPin* pin, 
		const PinData& data) override;
	virtual com::ErrCode OnSinkPinNegotiated(ISinkPin* pin,
		const std::string& cap) override;

	void OnTimeout();

private:
	com::ErrCode CreateSinkPin();

private:
	uint32_t m_sink_pin_index = 0;

	media::com::AudioCap m_sink_pin_cap;

	uint32_t m_audio_energy = 0;
	com::TimerId m_timer_id = INVALID_TIMER_ID;
};

}