#pragma once

#include <mutex>
#include <list>

#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "if-pin.h"
#include "if-element.h"
#include "common-struct.h"

#include "public/media-enum.h"

namespace jukey::stmr
{

//==============================================================================
// Sink Pin
//==============================================================================
class SinkPin 
	: public ISinkPin
	, public base::ProxyUnknown
	, public base::ComObjTracer
{
public:
	SinkPin(base::IComFactory* factory, const char* owner);
	~SinkPin();

	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IPin
	virtual PinType Type() override;
	virtual media::MediaType MType() override;
	virtual std::string Name() override;
	virtual std::string StreamId() override;
	virtual std::string Caps() override;
	virtual std::string Cap() override;
	virtual std::string ToStr() override;
	virtual bool Negotiated() override;
	virtual IElement* Element() override;
	virtual PinCaps PrepCaps() override;
	virtual PinCaps AvaiCaps() override;
	virtual com::ErrCode OnPinData(const PinData& data) override;
	virtual com::ErrCode OnPinMsg(IPin* pin, const PinMsg& msg) override;
	
	// ISinkPin
	virtual com::ErrCode Init(media::MediaType media_type, IElement* element,
		CSTREF pin_name, CSTREF caps, ISinkPinHandler* handler) override;
	virtual ISrcPin* SrcPin() override;
	virtual void SetSrcPin(ISrcPin* src_pin) override;
	virtual com::ErrCode UpdatePrepCaps(IPin* pin, const PinCaps& caps) override;
	virtual com::ErrCode SetNegotiateCap(IPin* pin, const std::string& cap) override;

private:
	com::ErrCode OnSetStreamId(const PinMsg& msg);
	com::ErrCode InitAvaiCaps();
	com::ErrCode SendMsgToSrcPins(const PinMsg& msg);

private:
	PinType m_pin_type = PinType::SINK;
	std::string m_pin_name = "sink-pin";
	media::MediaType m_media_type = media::MediaType::INVALID;
	
	IElement* m_element = nullptr;
	ISinkPinHandler* m_handler = nullptr;

	ISrcPin* m_src_pin = nullptr;
	
	std::string m_init_caps; // initialized capabilities
	std::string m_nego_cap;  // negotiated capability

	PinCaps m_avai_caps; // available capabilities
	PinCaps m_prep_caps; // prepared capabilities

	std::string m_stream_id;

	bool m_negotiated = false;
};

}