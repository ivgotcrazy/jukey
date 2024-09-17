#include "sink-pin.h"
#include "util-streamer.h"
#include "log.h"
#include "streamer-common.h"
#include "util-enum.h"


using namespace jukey::com;
using namespace jukey::media::util;

namespace
{

using namespace jukey::stmr;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
PinCaps MakePreparedCaps(const PinCaps& avai_caps, std::list<ISrcPin*> src_pins)
{
	PinCaps prep_caps;

	for (auto& cap : avai_caps) {
		for (auto src_pin : src_pins) {
			PinCaps caps = src_pin->AvaiCaps();
			if (avai_caps.end() != std::find(caps.begin(), caps.end(), cap)) {
				prep_caps.push_back(cap);
			}
		}
	}

	return prep_caps;
}

}

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SinkPin::SinkPin(base::IComFactory* factory, const char* owner)
  : base::ProxyUnknown(nullptr)
  , base::ComObjTracer(factory, CID_SINK_PIN, owner)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SinkPin::~SinkPin()
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode SinkPin::InitAvaiCaps()
{
	if (m_media_type == media::MediaType::AUDIO) {
		m_avai_caps.clear();
		if (ERR_CODE_OK != media::util::ParseAvaiAudioCaps(m_init_caps,
			m_avai_caps)) {
			LOG_ERR("[{}] Parse avaliable audio caps failed!", this->ToStr());
			return ERR_CODE_FAILED;
		}
	}
	else if (m_media_type == media::MediaType::VIDEO) {
		m_avai_caps.clear();
		if (ERR_CODE_OK != media::util::ParseAvaiVideoCaps(m_init_caps,
			m_avai_caps)) {
			LOG_ERR("[{}] Parse avaliable video caps failed!", this->ToStr());
			return ERR_CODE_FAILED;
		}
	}
	else {
		LOG_ERR("[{}] Invalid media type:{}", this->ToStr(), m_media_type);
		return ERR_CODE_INVALID_PARAM;
	}

	m_prep_caps = m_avai_caps;

	LOG_DBG("{} init avaliable caps:{}", ToStr(), PinCapsToStr(m_avai_caps));

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode SinkPin::Init(media::MediaType media_type, IElement* element,
	CSTREF pin_name, CSTREF caps, ISinkPinHandler* handler)
{
	if (!element) {
		LOG_ERR("Invalid element");
		return ERR_CODE_INVALID_PARAM;
	}

	if (!handler) {
		LOG_ERR("Invalid src pin handler");
		return ERR_CODE_INVALID_PARAM;
	}

	if (caps.empty()) {
		LOG_ERR("Invalid caps");
		return ERR_CODE_INVALID_PARAM;
	}

	m_media_type = media_type;
	m_element    = element;
	m_pin_name   = pin_name;
	m_init_caps  = caps;
	m_handler    = handler;

	if (ERR_CODE_OK != InitAvaiCaps()) {
		LOG_ERR("[{}] Init avaliable caps failed!", this->ToStr());
		return ERR_CODE_FAILED;
	}

	return com::ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* SinkPin::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_SINK_PIN) == 0) {
		return new SinkPin(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* SinkPin::NDQueryInterface(const char* riid)
{
	if (0 == strcmp(riid, IID_SINK_PIN)) {
		return static_cast<ISinkPin*>(this);
	}
	else {
		return ProxyUnknown::NDQueryInterface(riid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
PinType SinkPin::Type()
{
	return m_pin_type;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string SinkPin::Name()
{
	return m_pin_name;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string SinkPin::StreamId()
{
	return m_stream_id;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
media::MediaType SinkPin::MType()
{
	return m_media_type;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string SinkPin::Caps()
{
	return m_init_caps;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string SinkPin::Cap()
{
	return m_nego_cap;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
IElement* SinkPin::Element()
{
	return m_element;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode SinkPin::OnSetStreamId(const PinMsg& msg)
{
	m_stream_id = msg.msg_data.sp;

	m_handler->OnSinkPinMsg(this, msg);

	return com::ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string SinkPin::ToStr()
{
	return m_element->Name().append("|").append(m_pin_name);
}

//------------------------------------------------------------------------------
// @param pin - source pin
//------------------------------------------------------------------------------
ErrCode SinkPin::SetNegotiateCap(IPin* pin, const std::string& cap)
{
	if (!pin) {
		LOG_ERR("[{}] Invalid pin", this->ToStr());
		return ERR_CODE_INVALID_PARAM;
	}

	if (cap.empty()) {
		LOG_ERR("[{}] Empty cap", this->ToStr());
		return ERR_CODE_INVALID_PARAM;
	}

	LOG_INF("[{}] Set cap, src pin:{}, cap:{}", this->ToStr(), pin->ToStr(),
		media::util::Capper(cap));

	if (m_negotiated) {
		LOG_WRN("[{}] Renegotiate!", this->ToStr()); // TODO: right???
	}

	if (m_nego_cap == cap) {
		LOG_WRN("[{}] Set the same cap!", this->ToStr()); // TODO: right???
	}

	// Must set before call OnSinkPinNegotiated
	m_nego_cap = cap;
	m_negotiated = true;

	// Callback element
	if (ERR_CODE_OK != m_handler->OnSinkPinNegotiated(this, cap)) {
		LOG_ERR("[{}] Notify sink pin negotiated failed", this->ToStr());
		// Must set while failed
		m_nego_cap = "";
		m_negotiated = false;
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::vector<std::string> SinkPin::PrepCaps()
{
	return m_prep_caps;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::vector<std::string> SinkPin::AvaiCaps()
{
	return m_avai_caps;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode SinkPin::UpdatePrepCaps(IPin* pin, const PinCaps& caps)
{
	if (!pin) {
		LOG_ERR("[{}] Invalid pin", this->ToStr());
		return ERR_CODE_INVALID_PARAM;
	}

	if (caps.empty()) {
		LOG_ERR("[{}] Empty caps", this->ToStr());
		return ERR_CODE_INVALID_PARAM;
	}

	LOG_DBG("[{}] Update prepared caps, src pin:{}, caps:{}", this->ToStr(),
		pin->ToStr(), PinCapsToStr(caps));

	m_prep_caps = caps;

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode SinkPin::SendMsgToSrcPins(const PinMsg& msg)
{
	ErrCode result = ERR_CODE_OK;
	
	if (m_src_pin) {
		if (ERR_CODE_OK != m_src_pin->OnPinMsg(this, msg)) {
			LOG_ERR("[{}] Process message failed, src pin:{}, msg:{}", this->ToStr(),
				m_src_pin->Name(), msg.msg_type);
			result = ERR_CODE_FAILED;
		}
	}

	return result;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode SinkPin::OnPinMsg(IPin* pin, const PinMsg& msg)
{
	if (pin) { // forward message (from src pin)
		LOG_INF("[{}] Forward pin msg:{}", this->ToStr(),
			media::util::PIN_MSG_STR(msg.msg_type));

		if (msg.msg_type == PinMsgType::SET_STREAM) {
			m_stream_id = msg.msg_data.sp;
		}

		return m_handler->OnSinkPinMsg(this, msg);
	}
	else { // backward message (from element)
		LOG_INF("[{}] Backward pin msg:{}", this->ToStr(), 
			media::util::PIN_MSG_STR(msg.msg_type));

		return SendMsgToSrcPins(msg);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode SinkPin::OnPinData(const PinData& data)
{
	if (m_media_type != data.mt) {
		LOG_ERR("[{}] Unexpected media type:{}", this->ToStr(), data.mt);
		return com::ERR_CODE_FAILED;
	}

	if (!m_negotiated) {
		LOG_ERR("{} not negotiated!", this->ToStr());
		return com::ERR_CODE_FAILED;
	}

	if (m_handler) {
		return m_handler->OnSinkPinData(this, data);
	}
	else {
		LOG_ERR("[{}] Invalid handler!", this->ToStr());
		return com::ERR_CODE_FAILED;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ISrcPin* SinkPin::SrcPin()
{
	return m_src_pin;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SinkPin::Negotiated()
{
	return m_negotiated;
}

//------------------------------------------------------------------------------
// sink pin 允许连接多个 src pin ?
//------------------------------------------------------------------------------
void SinkPin::SetSrcPin(ISrcPin* src_pin)
{
	if (src_pin) {
		LOG_INF("[{}] Set src pin:{}|{}", this->ToStr(), src_pin->ToStr());

		if (m_src_pin) {
			LOG_ERR("[{}] Src pin already exists", this->ToStr());
			return;
		}

		if (src_pin->MType() != m_media_type) {
			LOG_ERR("[{}] Invalid src pin media type:{}", this->ToStr(),
				src_pin->MType());
			return;
		}

		m_src_pin = src_pin;
		m_handler->OnSinkPinConnectState(this, true);
	}
	else {
		LOG_INF("[{}] Reset src pin", this->ToStr());

		m_prep_caps.clear();
		InitAvaiCaps();

		m_src_pin = nullptr;
		m_handler->OnSinkPinConnectState(this, false);
	}
}

}