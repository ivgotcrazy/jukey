#pragma once

#include <string>
#include <list>
#include <memory>

#include "component.h"
#include "common-define.h"
#include "common-enum.h"
#include "common-struct.h"

#include "public/media-enum.h"
#include "public/media-struct.h"

namespace jukey::stmr
{

// Component identity and interface identify
#define CID_SRC_PIN "cid-src-pin"
#define IID_SRC_PIN "iid-src-pin"
#define CID_SINK_PIN "cid-sink-pin"
#define IID_SINK_PIN "iid-sink-pin"

//==============================================================================
// Pin type
//==============================================================================
enum class PinType
{
	INVALID = 0,
	SRC     = 1,
	SINK    = 2
};

enum class PinState
{
	INVALID     = 0,
	NEGOTIATING = 1,
	NEGOTIATED  = 2,
};

//==============================================================================
// Pin message type
//==============================================================================
enum class PinMsgType
{
	INVALID     = 0,

	ELE_START   = 1,
	ELE_PAUSE   = 2,
	ELE_RESUME  = 3,
	ELE_STOP    = 4,
	SET_STREAM  = 5,
	END_STREAM  = 6,
	NEGOTIATE   = 7,
	NO_PENDING  = 8,
	SET_PENDING = 9,
	FLUSH_DATA  = 10,
};

//==============================================================================
// Pin message data
//==============================================================================
struct PinMsgData
{
	PinMsgData() {}
	PinMsgData(void* p2) : vp(p2) {}
	PinMsgData(const std::string& p1) : sp(p1) {}
	PinMsgData(const std::string& p1, void* p2) :sp(p1), vp(p2) {}

	std::string sp;     // string parameter
	void* vp = nullptr; // void* parameter
};

//==============================================================================
// Pin message
//==============================================================================
struct PinMsg
{
	PinMsg() {}
	PinMsg(PinMsgType type) : msg_type(type) {}
	PinMsg(PinMsgType type, const PinMsgData& data) 
		: msg_type(type), msg_data(data) {}
	PinMsg(PinMsgType type, void* p2) 
		: msg_type(type), msg_data(p2) {}
	PinMsg(PinMsgType type, const std::string& p1) 
		: msg_type(type), msg_data(p1) {}
	PinMsg(PinMsgType type, const std::string& p1, void* p2) 
		: msg_type(type), msg_data(p1, p2) {}

	PinMsgType msg_type = PinMsgType::INVALID;
	PinMsgData msg_data;
};

//==============================================================================
// Pin data
//==============================================================================
struct PinData
{
	PinData() {}

	PinData(media::MediaType type) : mt(type) 
	{
		if (type == media::MediaType::AUDIO) {
			media_para.reset(new media::AudioFramePara());
		}
		else if (type == media::MediaType::VIDEO) {
			media_para.reset(new media::VideoFramePara());
		}
	}

	PinData(media::MediaType type, uint32_t len) : PinData(type)
	{
		media_data[0].data.reset(new uint8_t[len]);
		media_data[0].total_len = len;
		data_count = 1;
	}

	PinData(media::MediaType type, uint8_t* data, uint32_t len)
		: PinData(type, len)
	{
		memcpy(media_data[0].data.get(), data, len);
		media_data[0].data_len = len;
		data_count = 1;
	}

	media::MediaType mt = media::MediaType::INVALID;

	int64_t  pts = 0; // display timestamp
	int64_t  dts = 0; // decode timestamp
	int64_t  pos = 0; // position
	uint64_t drt = 0; // duration
	uint64_t syn = 0; // synchronize id
	int32_t  tbn = 0; // time_base numerator 
	int32_t  tbd = 0; // time_base denominator

	com::Buffer media_data[8];
	uint32_t data_count = 0;

	// AudioFramePara or VideoFramePara
	std::shared_ptr<void> media_para;
};
typedef std::shared_ptr<PinData> PinDataSP;

//==============================================================================
//
//==============================================================================
typedef std::vector<std::string> PinCaps;

class IElement;
//==============================================================================
// Elements should be linked with pins
// TODO: Whether need a pin index?
//==============================================================================
class IPin : public base::IUnknown
{
public:
	//
	// Pin type
	//
	virtual PinType Type() = 0;

	//
	// Pin media type
	//
	virtual media::MediaType MType() = 0;

	//
	// Pin stream ID
	//
	virtual std::string StreamId() = 0;

	//
	// Pin belongs to
	//
	virtual IElement* Element() = 0;

	//
	// Pin name
	//
	virtual std::string Name() = 0;

	//
	// Pin capabilities
	//
	virtual std::string Caps() = 0;

	//
	// Negotiated pin capability(paramenters)
	//
	virtual std::string Cap() = 0;

	//
	// element_name | pin_name
	//
	virtual std::string ToStr() = 0;

	//
	// Check negotiate state
	//
	virtual bool Negotiated() = 0;

	//
	// Get prepared capabilities
	//
	virtual PinCaps PrepCaps() = 0;

	//
	// Get available capabilities
	//
	virtual PinCaps AvaiCaps() = 0;

	//
	// Input pin data
	//
	virtual com::ErrCode OnPinData(const PinData& data) = 0;

	//
	// Input pin message
	//
	virtual com::ErrCode OnPinMsg(IPin* pin, const PinMsg& msg) = 0;
};

class ISrcPin;
//==============================================================================
// Src pin event handler
//==============================================================================
class ISrcPinHandler
{
public:
	//
	// Src pin message callback
	//
	virtual com::ErrCode OnSrcPinMsg(ISrcPin* pin, const PinMsg& msg) = 0;

	//
	// Notify sink pin size changed
	//
	virtual void OnSrcPinConnectState(ISrcPin* pin, bool add) = 0;

	//
	// Notify src pin negotiated
	//
	virtual com::ErrCode OnSrcPinNegotiated(ISrcPin* src_pin, 
		const std::string& cap) = 0;
};

class ISinkPin;
//==============================================================================
// Interface of src pin
//==============================================================================
class ISrcPin : public IPin
{
public:
	//
	// Initialize
	//
	virtual com::ErrCode Init(media::MediaType media_type, 
		IElement* element,
		CSTREF pin_name,
		CSTREF caps,
		ISrcPinHandler* handler) = 0;

	//
	// One src pin can add more than one sink Pin
	//
	virtual com::ErrCode AddSinkPin(ISinkPin* pin) = 0;

	//
	// Remove sink pin link
	//
	virtual com::ErrCode RemoveSinkPin(CSTREF pin_name) = 0;

	//
	// Get sink pin by name
	//
	virtual ISinkPin* SinkPin(CSTREF pin_name) = 0;

	//
	// Get all sink pins
	//
	virtual std::list<ISinkPin*> SinkPins() = 0;

	//
	// Start to negotiate
	//
	virtual com::ErrCode Negotiate() = 0;

	//
	// Update available capabilities
	//
	virtual com::ErrCode UpdateAvaiCaps(const PinCaps& caps) = 0;
};

//==============================================================================
// Sink pin event handler
//==============================================================================
class ISinkPinHandler
{
public:
	//
	// Sink pin message callback
	//
	virtual com::ErrCode OnSinkPinMsg(ISinkPin* pin, const PinMsg& msg) = 0;

	//
	// Sin pin data callback
	//
	virtual com::ErrCode OnSinkPinData(ISinkPin* pin, const PinData& data) = 0;

	//
	// Notify sink pin negotiated
	//
	virtual com::ErrCode OnSinkPinNegotiated(ISinkPin* pin,
		const std::string& cap) = 0;

	//
	// Notify sink pin conneceted or disconnected
	//
	virtual void OnSinkPinConnectState(ISinkPin* sink_pin, bool connected) = 0;
};

//==============================================================================
// Interface of sink pin
//==============================================================================
class ISinkPin : public IPin
{
public:
	//
	// Initialize
	//
	virtual com::ErrCode Init(media::MediaType media_type, 
		IElement* element,
		CSTREF pin_name, 
		CSTREF caps, 
		ISinkPinHandler* handler) = 0;

	//
	// Get src pin
	//
	virtual ISrcPin* SrcPin() = 0;


	//
	// Add src pin, nullptr means reset
	//
	virtual void SetSrcPin(ISrcPin* src_pin) = 0;

	//
	// Set capability
	//
	virtual com::ErrCode SetNegotiateCap(IPin* pin, const std::string& cap) = 0;

	//
	// Update prepared capabilities
	//
	virtual com::ErrCode UpdatePrepCaps(IPin* pin, const PinCaps& caps) = 0;
};

}