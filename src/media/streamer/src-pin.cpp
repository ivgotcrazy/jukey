#include "src-pin.h"
#include "cap-negotiate.h"
#include "util-streamer.h"
#include "common/util-common.h"
#include "if-pipeline.h"
#include "pipeline-msg.h"
#include "log.h"
#include "streamer-common.h"
#include "util-enum.h"


using namespace jukey::util;
using namespace jukey::com;
using namespace jukey::media::util;

namespace
{

using namespace jukey::stmr;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
PinCaps MakePreparedCaps(const PinCaps& avai_caps, std::list<ISinkPin*> sink_pins)
{
	PinCaps prep_caps;

	for (auto& cap : avai_caps) {
		bool all_sink_pin_found = true;
		for (auto sink_pin : sink_pins) {
			PinCaps caps = sink_pin->AvaiCaps();
			if (std::find(caps.begin(), caps.end(), cap) == caps.end()) {
				all_sink_pin_found = false;
				break;
			}
		}
		if (all_sink_pin_found) {
			prep_caps.push_back(cap);
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
SrcPin::SrcPin(base::IComFactory* factory, const char* owner)
  : base::ProxyUnknown(nullptr)
  , base::ComObjTracer(factory, CID_SRC_PIN, owner)
{
	
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SrcPin::~SrcPin()
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* SrcPin::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_SRC_PIN) == 0) {
		return new SrcPin(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* SrcPin::NDQueryInterface(const char* riid)
{
	if (0 == strcmp(riid, IID_SRC_PIN)) {
		return static_cast<ISrcPin*>(this);
	}
	else {
		return ProxyUnknown::NDQueryInterface(riid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode SrcPin::InitAvaiCaps()
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

	LOG_DBG("{} init avaliable caps:{}", this->ToStr(), PinCapsToStr(m_avai_caps));

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode SrcPin::Init(media::MediaType media_type, IElement* element,
	CSTREF pin_name, CSTREF caps, ISrcPinHandler* handler)
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

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
PinType SrcPin::Type()
{
	return m_pin_type;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string SrcPin::StreamId()
{
	return m_stream_id;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string SrcPin::Name()
{
	return m_pin_name;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
media::MediaType SrcPin::MType()
{
	return m_media_type;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string SrcPin::Caps()
{
	return m_init_caps;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string SrcPin::Cap()
{
	return m_nego_cap;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
IElement* SrcPin::Element()
{
	return m_element;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode SrcPin::Negotiate()
{
	LOG_INF("[{}] Negotiate", this->ToStr());

	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_sink_pins.empty()) {
		LOG_WRN("[{}] No sink pin!", this->ToStr());
		return ERR_CODE_OK; // TODO:
	}

	if (m_prep_caps.empty()) {
		LOG_WRN("[{}] No prepared caps!", this->ToStr());
		return ERR_CODE_FAILED;
	}

	m_negotiated = false;
	bool result = true;

	// Traverse all prepared caps, try to find a cap
	for (const auto& prep_cap : m_prep_caps) {
		for (ISinkPin* sink_pin : m_sink_pins) { // match all sink pins
			LOG_INF("[{}] Set cap, sink pin:{}, cap:{}", this->ToStr(),
				sink_pin->ToStr(), media::util::Capper(prep_cap));

			if (ERR_CODE_OK != sink_pin->SetNegotiateCap(this, prep_cap)) {
				LOG_ERR("[{}] Set sink pin cap failed", this->ToStr());
				result = false;
				break;
			}
			// TODO: SetNegotiateCap如果失败有副作用
		}
			
		if (result) {
			m_nego_cap = prep_cap;
			m_negotiated = true;
			break;
		}
	}

	if (!result) {
		LOG_ERR("[{}] Set sink pin cap failed!", this->ToStr());
		return ERR_CODE_FAILED;
	}

	// 通知协商成功
	m_handler->OnSrcPinNegotiated(this, m_nego_cap);

	LOG_INF("[{}] Negotiated cap:{}", this->ToStr(), 
		media::util::Capper(m_nego_cap));

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::vector<std::string> SrcPin::PrepCaps()
{
	return m_prep_caps;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::vector<std::string> SrcPin::AvaiCaps()
{
	return m_avai_caps;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode SrcPin::UpdateAvaiCaps(const PinCaps& caps)
{
	LOG_INF("[{}] Update available caps:{}", this->ToStr(), PinCapsToStr(caps));

	if (caps.empty()) {
		LOG_ERR("[{}] Empty caps", this->ToStr());
		return ERR_CODE_FAILED;
	}

	m_avai_caps = caps;

	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_sink_pins.empty()) {
		LOG_INF("[{}] Empty sink pins, need not to process", this->ToStr());
		return ERR_CODE_OK;
	}

	// Use prepared caps
	PinCaps prep_caps = MakePreparedCaps(caps, m_sink_pins);
	if (prep_caps.empty()) {
		LOG_ERR("[{}] Empty prepared caps", this->ToStr());
		return ERR_CODE_FAILED;
	}

	LOG_INF("[{}] Make prepared caps:{}", this->ToStr(), PinCapsToStr(prep_caps));

	for (auto sink_pin : m_sink_pins) {
		if (ERR_CODE_OK != sink_pin->UpdatePrepCaps(this, prep_caps)) {
			LOG_ERR("[{}] Update prepared caps failed, sink pin:{}", this->ToStr(),
				sink_pin->ToStr());
			return ERR_CODE_FAILED;
		}
	}

	m_prep_caps = prep_caps;

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode SrcPin::OnPinData(const PinData& data)
{
	if (m_media_type != data.mt) {
		LOG_ERR("[{}] Invalid media type:{}", this->ToStr(), data.mt);
		return ERR_CODE_FAILED;
	}

	if (!m_negotiated && !m_logged) {
		LOG_ERR("[{}] OnPinData, but src pin not negotiated", this->ToStr());
		m_logged = true;
		return ERR_CODE_FAILED;
	}

	bool has_error = false;

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		for (auto item : m_sink_pins) {
			ErrCode result = item->OnPinData(data);
			if (result != ERR_CODE_OK) {
				has_error = true;
			}
		}
	}

	return has_error ? ERR_CODE_FAILED : ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode SrcPin::SendMsgToSinkPins(const PinMsg& msg)
{
	ErrCode result = ERR_CODE_OK;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		for (auto sink_pin : m_sink_pins) {
			if (ERR_CODE_OK != sink_pin->OnPinMsg(this, msg)) {
				LOG_ERR("[{}] Process message failed, sink pin:{}, msg:{}", 
					this->ToStr(),
					sink_pin->ToStr(), 
					media::util::PIN_MSG_STR(msg.msg_type));
				result = ERR_CODE_FAILED;
			}
		}
	}

	return result;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode SrcPin::OnPinMsg(IPin* pin, const PinMsg& msg)
{
	if (pin) { // backward message (from sink pin)
		LOG_INF("[{}] Backward pin msg:{}", this->ToStr(), 
			media::util::PIN_MSG_STR(msg.msg_type));

		return m_handler->OnSrcPinMsg(this, msg);
	}
	else { // forward message (from element) 
		LOG_INF("[{}] Forward pin msg:{}", this->ToStr(),
			media::util::PIN_MSG_STR(msg.msg_type));

		if (msg.msg_type == PinMsgType::SET_STREAM) {
			m_stream_id = msg.msg_data.sp;
		}

		return SendMsgToSinkPins(msg);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string SrcPin::ToStr()
{
	return m_element->Name().append("|").append(m_pin_name);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SrcPin::Negotiated()
{
	return m_negotiated;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode SrcPin::AddSinkPin(ISinkPin* sink_pin)
{
	if (!sink_pin) {
		LOG_INF("[{}] Invalid sink pin!", this->ToStr());
		return ERR_CODE_INVALID_PARAM;
	}

	LOG_INF("[{}] Add sink pin, sink pin:{}", this->ToStr(), sink_pin->ToStr());

	if (sink_pin->MType() != m_media_type) {
		LOG_ERR("[{}] Invalid media type:{}", this->ToStr(), sink_pin->MType());
		return ERR_CODE_FAILED;
	}

	if (m_avai_caps.empty()) {
		LOG_ERR("[{}] Empty available caps", this->ToStr());
		return ERR_CODE_FAILED;
	}

	{
		std::lock_guard<std::mutex> lock(m_mutex);

		// Check add the same sink pin repeatly
		for (auto item : m_sink_pins) {
			if (item == sink_pin) {
				LOG_ERR("[{}] Sink pin already exists", this->ToStr());
				return ERR_CODE_FAILED;
			}
		}

		// SrcPin 已经协商过，不再重复协商，SrcPin 直接设置 SinkPin 的能力
		if (m_negotiated) {
			std::vector<std::string> caps{ m_nego_cap };
			std::list<ISinkPin*> sink_pins{ sink_pin };

			PinCaps prep_caps = MakePreparedCaps(caps, sink_pins);
			if (prep_caps.empty()) {
				LOG_ERR("Make prepared caps failed!");
				return ERR_CODE_FAILED;
			}

			if (ERR_CODE_OK != sink_pin->UpdatePrepCaps(this, prep_caps)) {
				LOG_ERR("[{}] Update prepared caps failed, sink pin:{}", 
					this->ToStr(), sink_pin->ToStr());
				return ERR_CODE_FAILED;
			}

			if (ERR_CODE_OK != sink_pin->SetNegotiateCap(this, m_nego_cap)) {
				LOG_ERR("Set cap failed!");
				return ERR_CODE_FAILED;
			}
		}
		else {
			// 使用临时容器将所有 SinkPin 加入进来
			std::list<ISinkPin*> sink_pins = m_sink_pins;
			sink_pins.push_back(sink_pin);

			// 看是否可以找到公共能力集
			PinCaps prep_caps = MakePreparedCaps(m_avai_caps, sink_pins);

			// 如果找不到公共能力集，则打印相关信息并返回失败
			if (prep_caps.empty()) {
				LOG_ERR("[{}] Make prepared caps failed, avai caps:{}", 
					this->ToStr(),
					media::util::PinCapsToStr(m_avai_caps));
				for (const auto& sink_pin : sink_pins) {
					LOG_ERR("Sink pin:{} avai caps:", sink_pin->ToStr(),
						media::util::PinCapsToStr(sink_pin->AvaiCaps()));
				}
				return ERR_CODE_FAILED;
			}

			LOG_DBG("[{}] Make prepared caps:{}", this->ToStr(), 
				PinCapsToStr(prep_caps));

			// 找到公共能力集，设置到所有 SinkPin
			for (auto item : sink_pins) {
				LOG_INF("[{}] Update sink pin prepared caps, sink pin:{}", 
					this->ToStr(), item->ToStr());

				if (ERR_CODE_OK != item->UpdatePrepCaps(this, prep_caps)) {
					LOG_ERR("[{}] Update prepared caps failed, sink pin:{}", 
						this->ToStr(), item->ToStr());
					return ERR_CODE_FAILED;
				}
			}

			// 更新公共能力集
			m_prep_caps = prep_caps;
		}

		// 加入新的 SinkPin
		m_sink_pins.push_back(sink_pin);

		// 设置 SrcPin
		sink_pin->SetSrcPin(this);
	}

	if (ERR_CODE_OK != sink_pin->OnPinMsg(this, PinMsg(PinMsgType::SET_STREAM, 
		m_stream_id))) {
		LOG_ERR("Set stream failed, sink pin:{}", sink_pin->Name());
		return ERR_CODE_FAILED;
	}

	// 通知 element SinPin 数量变化
	m_handler->OnSrcPinConnectState(this, true);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// TODO: 
//------------------------------------------------------------------------------
ErrCode SrcPin::RemoveSinkPin(CSTREF pin_name)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto iter = m_sink_pins.begin(); iter != m_sink_pins.end(); ++iter) {
		if ((*iter)->Name() == pin_name) {
			m_sink_pins.erase(iter);
			if (m_handler) {
				m_handler->OnSrcPinConnectState(this, false);
			}
			
			if (m_sink_pins.empty()) {
				m_negotiated = false;
				m_nego_cap = "";
				m_handler->OnSrcPinNegotiated(this, "");
			}

			return ERR_CODE_OK;
		}
	}

	return ERR_CODE_FAILED;
}

//------------------------------------------------------------------------------
// TODO: shared_ptr
//------------------------------------------------------------------------------
ISinkPin* SrcPin::SinkPin(CSTREF pin_name)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto item : m_sink_pins) {
		if (item->Name() == pin_name) {
			return item;
		}
	}

	return nullptr;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::list<ISinkPin*> SrcPin::SinkPins()
{
	return m_sink_pins;
}

}