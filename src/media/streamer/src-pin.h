#pragma once

#include <list>
#include <mutex>

#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "common-struct.h"
#include "if-pin.h"
#include "if-element.h"

#include "public/media-enum.h"


namespace jukey::stmr
{

//==============================================================================
// Src Pin
//==============================================================================
class SrcPin 
	: public ISrcPin
	, public base::ProxyUnknown
	, public base::ComObjTracer
{
public:
	SrcPin(base::IComFactory* factory, const char* owner);
	~SrcPin();

	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IPin
	virtual PinType Type() override;
	virtual media::MediaType MType() override;
	virtual std::string StreamId() override;
	virtual std::string Name() override;
	virtual std::string Caps() override;
	virtual std::string Cap() override;
	virtual std::string ToStr() override;
	virtual bool Negotiated() override;
	virtual IElement* Element() override;
	virtual PinCaps PrepCaps() override;
	virtual PinCaps AvaiCaps() override;
	virtual com::ErrCode OnPinData(const PinData& data) override;
	virtual com::ErrCode OnPinMsg(IPin* pin, const PinMsg& msg) override;

	// ISrcPin
	virtual com::ErrCode Init(media::MediaType media_type, IElement* element,
		CSTREF pin_name, CSTREF caps, ISrcPinHandler* handler) override;
	virtual com::ErrCode AddSinkPin(ISinkPin* sink_pin) override;
	virtual com::ErrCode RemoveSinkPin(CSTREF pin_name) override;
	virtual ISinkPin* SinkPin(CSTREF pin_name) override;
	virtual std::list<ISinkPin*> SinkPins() override;
	virtual com::ErrCode Negotiate() override;
	virtual com::ErrCode UpdateAvaiCaps(const PinCaps& caps) override;

private:
	com::ErrCode SendMsgToSinkPins(const PinMsg& msg);
	com::ErrCode InitAvaiCaps();

private:
	ISrcPinHandler* m_handler = nullptr;

	PinType m_pin_type = PinType::SRC;
	media::MediaType m_media_type = media::MediaType::INVALID;
	std::string m_pin_name = "src-pin";
	
	std::mutex m_mutex;
	std::list<ISinkPin*> m_sink_pins;

	IElement* m_element = nullptr;

	// 初始能力集（initialized capabilities）
	// 虽然初始能力集会赋值可用能力集，但可用能力集会被更新，更新后无法获得初始能力集，
	// 因此初始能力集仍然有存在的必要
	std::string m_init_caps;

	// 最终协商的能力（negotiated capability）
	std::string m_nego_cap;

	// 可用能力集（available capabilities）
	// 当 sink pin 协商完成时，element 会更新 src pin 的可用能力集
	PinCaps m_avai_caps;

	// 备用能力集（prepared capabilities）
	// 可被选为最终协商结果的能力集
	PinCaps m_prep_caps;

	bool m_started = false;
	bool m_negotiated = false;

	std::string m_stream_id;

	bool m_logged = false;
};

}