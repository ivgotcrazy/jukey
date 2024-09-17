#include "element-base.h"
#include "common/util-common.h"
#include "util-enum.h"
#include "util-streamer.h"
#include "pipeline-msg.h"


using namespace jukey::com;
using namespace jukey::media::util;

////////////////////////////////////////////////////////////////////////////////
// Use element logger

#define LOG_DBG(...)                                            \
  if (m_logger && m_logger->GetLogger()) {                      \
    SPDLOG_LOGGER_DEBUG(m_logger->GetLogger(), __VA_ARGS__);    \
  }

#define LOG_INF(...)                                            \
  if (m_logger && m_logger->GetLogger()) {                      \
    SPDLOG_LOGGER_INFO(m_logger->GetLogger(), __VA_ARGS__);     \
  }

#define LOG_WRN(...)                                            \
  if (m_logger && m_logger->GetLogger()) {                      \
    SPDLOG_LOGGER_WARN(m_logger->GetLogger(), __VA_ARGS__);     \
  }

#define LOG_ERR(...)                                            \
  if (m_logger && m_logger->GetLogger()) {                      \
    SPDLOG_LOGGER_ERROR(m_logger->GetLogger(), __VA_ARGS__);    \
  }

#define LOG_CRT(...)                                            \
  if (m_logger && m_logger->GetLogger()) {                      \
    SPDLOG_LOGGER_CRITICAL(m_logger->GetLogger(), __VA_ARGS__); \
  }
////////////////////////////////////////////////////////////////////////////////

namespace jukey::media::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ElementBase::ElementBase(base::IComFactory* factory) : m_factory(factory)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ElementBase::~ElementBase()
{
	for (auto item : m_src_pins) {
		item->Release();
	}
	m_src_pins.clear();

	for (auto item : m_sink_pins) {
		item->Release();
	}
	m_sink_pins.clear();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::Init(stmr::IPipeline* pipeline, IProperty* props)
{
	LOG_INF("[{}] Init", m_ele_name);

	if (!pipeline) {
		LOG_ERR("Invalid pipeline!");
		return ERR_CODE_INVALID_PARAM;
	}
	m_pipeline = pipeline;

	if (m_ele_state != stmr::EleState::INVALID) {
		LOG_ERR("Invalid element state:{}", m_ele_state);
		return ERR_CODE_FAILED;
	}

	if (!m_pins_created) {
		LOG_ERR("Create pins failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != DoInit(props)) {
		LOG_ERR("DoInit failed!");
		return ERR_CODE_FAILED;
	}

	m_pipeline->SubscribeMsg(stmr::PlMsgType::START_ELEMENT, this);
	m_pipeline->SubscribeMsg(stmr::PlMsgType::STOP_ELEMENT, this);
	m_pipeline->SubscribeMsg(stmr::PlMsgType::PAUSE_ELEMENT, this);
	m_pipeline->SubscribeMsg(stmr::PlMsgType::RESUME_ELEMENT, this);

	m_ele_state = stmr::EleState::INITED;

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::Start()
{
	LOG_INF("[{}] Start", m_ele_name);

	if (m_ele_state == stmr::EleState::RUNNING) {
		LOG_WRN("Element is already running!");
		return ERR_CODE_OK;
	}

	if (m_ele_state != stmr::EleState::INITED) {
		LOG_ERR("Invalid element state:{}", m_ele_state);
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != DoStart()) {
		LOG_ERR("DoStart failed!");
		return ERR_CODE_FAILED;
	}

	m_ele_state = stmr::EleState::RUNNING;

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::Pause()
{
	LOG_INF("[{}] Pause", m_ele_name);

	if (m_ele_state == stmr::EleState::PAUSED) {
		LOG_WRN("Element is already paused!");
		return ERR_CODE_OK;
	}

	if (m_ele_state != stmr::EleState::RUNNING) {
		LOG_ERR("Invalid element state:{}", m_ele_state);
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != DoPause()) {
		LOG_ERR("DoPause failed!");
		return ERR_CODE_FAILED;
	}

	m_ele_state = stmr::EleState::PAUSED;

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::Resume()
{
	LOG_INF("[{}] Resume", m_ele_name);

	// TODO:
	if (m_ele_state == stmr::EleState::INITED) {
		return Start();
	}

	if (m_ele_state == stmr::EleState::RUNNING) {
		LOG_WRN("Element is already running!");
		return ERR_CODE_OK;
	}

	if (m_ele_state != stmr::EleState::PAUSED) {
		LOG_ERR("Invalid element state:{}", m_ele_state);
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != DoResume()) {
		LOG_ERR("DoResume failed!");
		return ERR_CODE_FAILED;
	}

	m_ele_state = stmr::EleState::RUNNING;

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::Stop()
{
	LOG_INF("[{}] Stop", m_ele_name);

	if (m_ele_state == stmr::EleState::STOPED) {
		LOG_WRN("Element is already stoped!");
		return ERR_CODE_OK;
	}

	if (m_ele_state != stmr::EleState::INITED &&
		m_ele_state != stmr::EleState::RUNNING &&
		m_ele_state != stmr::EleState::PAUSED) {
		LOG_ERR("Invalid element state:{}", m_ele_state);
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != DoStop()) {
		LOG_ERR("Do stop failed!");
		return ERR_CODE_FAILED;
	}

	m_ele_state = stmr::EleState::STOPED;

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
stmr::EleState ElementBase::State()
{
	return m_ele_state;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string ElementBase::Name()
{
	return m_ele_name;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
stmr::EleMainType ElementBase::MainType()
{
	return m_main_type;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
stmr::EleSubType ElementBase::SubType()
{
	return m_sub_type;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
stmr::EleMediaType ElementBase::MType()
{
	return m_media_type;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::vector<stmr::ISrcPin*> ElementBase::SrcPins()
{
	return m_src_pins;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::vector<stmr::ISinkPin*> ElementBase::SinkPins()
{
	return m_sink_pins;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::DoStartElement()
{
	LOG_INF("[{}] DoStartElement", m_ele_name);

	return Start();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::DoInit(jukey::com::IProperty* props)
{
	LOG_INF("[{}] DoInit", m_ele_name);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::DoStart() 
{
	LOG_INF("[{}] DoStart", m_ele_name);

	return ERR_CODE_OK; 
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::DoPause() 
{
	LOG_INF("[{}] DoPause", m_ele_name);

	return ERR_CODE_OK; 
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::DoResume() 
{
	LOG_INF("[{}] DoResume", m_ele_name);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::DoStop() 
{
	LOG_INF("[{}] DoStop", m_ele_name);

	return ERR_CODE_OK; 
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::OnPinStartMsg(stmr::ISinkPin* pin, const stmr::PinMsg& msg)
{
	LOG_INF("[{}] OnPinStartMsg", m_ele_name);

	return DoStartElement();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::DoResumeElement()
{
	LOG_INF("[{}] DoResumeElement", m_ele_name);

	return Resume();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::OnPinResumeMsg(stmr::ISinkPin* pin, const stmr::PinMsg& msg)
{
	LOG_INF("[{}] OnPinResumeMsg", m_ele_name);

	return DoResumeElement();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::DoStopElement()
{
	LOG_INF("[{}] DoStopElement", m_ele_name);

	if (ERR_CODE_OK != Stop()) {
		LOG_ERR("Stop element failed!");
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::OnPinStopMsg(stmr::ISinkPin* pin, const stmr::PinMsg& msg)
{
	LOG_INF("[{}] OnPinStopMsg", m_ele_name);

	return DoStopElement();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::DoPauseElement()
{
	LOG_INF("[{}] DoPauseElement", m_ele_name);

	if (ERR_CODE_OK != Pause()) {
		LOG_ERR("Pause element failed!");
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::OnPinPauseMsg(stmr::ISinkPin* pin, const stmr::PinMsg& msg)
{
	LOG_INF("[{}] OnPinResumeMsg", m_ele_name);

	return DoPauseElement();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::TransferSrcPinMsg(const stmr::PinMsg& msg)
{
	for (auto sink_pin : m_sink_pins) {
		if (ERR_CODE_OK != sink_pin->OnPinMsg(nullptr, msg)) {
			LOG_WRN("Transfer sink msg failed, src_pin:{}", sink_pin->ToStr());
		}
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::TransferSinkPinMsg(const stmr::PinMsg& msg)
{
	for (auto src_pin : m_src_pins) {
		if (ERR_CODE_OK != src_pin->OnPinMsg(nullptr, msg)) {
			LOG_WRN("Transfer src msg failed, src_pin:{}", src_pin->ToStr());
		}
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
stmr::IPipeline* ElementBase::Pipeline()
{
	return m_pipeline;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::ProcSinkPinMsg(stmr::ISinkPin* pin, const stmr::PinMsg& msg)
{
	LOG_INF("[{}] ProcSinkPinMsg", m_ele_name);

	return ERR_CODE_MSG_NO_PROC;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::ProcSrcPinMsg(stmr::ISrcPin* pin, const stmr::PinMsg& msg)
{
	LOG_INF("[{}] ProcSrcPinMsg", m_ele_name);

	return ERR_CODE_MSG_NO_PROC;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::OnSinkPinMsg(stmr::ISinkPin* pin, const stmr::PinMsg& msg)
{
	LOG_INF("[{}] Sink pin msg:{}", pin->ToStr(), 
		PIN_MSG_STR((stmr::PinMsgType)msg.msg_type));

	//std::lock_guard<std::recursive_mutex> lock(m_mutex);

	ErrCode result = ProcSinkPinMsg(pin, msg);
	if (ERR_CODE_MSG_NO_PROC != result) {
		if (result == ERR_CODE_MSG_PROC_OK) {
			return ERR_CODE_OK;
		}
		else if (result == ERR_CODE_MSG_PROC_FAILED) {
			return ERR_CODE_FAILED;
		}
		else {
			LOG_INF("Proc sink pin msg result:{}", result);
		}
	}

	switch (msg.msg_type) {
	case stmr::PinMsgType::ELE_START:
		return OnPinStartMsg(pin, msg);
	case stmr::PinMsgType::ELE_PAUSE:
		return OnPinPauseMsg(pin, msg);
	case stmr::PinMsgType::ELE_RESUME:
		return OnPinResumeMsg(pin, msg);
	case stmr::PinMsgType::ELE_STOP:
		return OnPinStopMsg(pin, msg);
	default:
		return TransferSinkPinMsg(msg);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::OnSinkPinData(stmr::ISinkPin* pin, const stmr::PinData& data)
{
	LOG_INF("[{}] Sink pin data", m_ele_name);

	return ERR_CODE_FAILED;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode
ElementBase::OnSinkPinNegotiated(stmr::ISinkPin* pin, const std::string& cap)
{
	LOG_INF("[{}] Sink pin negotiated", m_ele_name);

	return ERR_CODE_FAILED;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ElementBase::OnSinkPinConnectState(stmr::ISinkPin* pin, bool connected)
{
	LOG_INF("[{}] OnSinkPinConnectState:{}", m_ele_name, connected);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::OnSrcPinMsg(stmr::ISrcPin* pin, const stmr::PinMsg& msg)
{
	LOG_INF("[{}] Src pin msg:{}", pin->ToStr(),
		PIN_MSG_STR((stmr::PinMsgType)msg.msg_type));

	ErrCode result = ProcSrcPinMsg(pin, msg);
	if (ERR_CODE_MSG_NO_PROC != result) {
		if (result == ERR_CODE_MSG_PROC_OK) {
			return ERR_CODE_OK;
		}
		else if (result == ERR_CODE_MSG_PROC_FAILED) {
			return ERR_CODE_FAILED;
		}
		else {
			LOG_INF("Proc sink pin msg result:{}", result);
		}
	}

	return TransferSrcPinMsg(msg);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ElementBase::ShouldMakeNegotiate()
{
	if (m_main_type == stmr::EleMainType::SRC) {
		return true;
	}

	if (m_sink_pins.empty()) {
		LOG_ERR("No sink pin!");
		return false;
	}

	for (auto sink_pin : m_sink_pins) {
		if (!sink_pin->Negotiated()) {
			return false;
		}
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ElementBase::OnSrcPinConnectState(stmr::ISrcPin* src_pin, bool add)
{
	LOG_INF("OnSrcPinConnectState, pin:{}, add:{}", src_pin->Name(), add);

	if (ShouldMakeNegotiate()) {
		if (add && src_pin->SinkPins().size() == 1) {
			if (src_pin->Negotiate() != ErrCode::ERR_CODE_OK) {
				LOG_ERR("Negotiate failed");
			}
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::OnSrcPinNegotiated(stmr::ISrcPin* src_pin,
	const std::string& cap)
{
	LOG_INF("OnSrcPinConnectState, pin:{}, cap:{}", src_pin->Name(), cap);

	return ErrCode::ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ElementBase::OnStartElementMsg(const CommonMsg& msg)
{
	if (!msg.dst.empty() && msg.dst != m_ele_name) {
		return;
	}

	LOG_INF("OnStartElementMsg, element:{}", m_ele_name);

	if (ERR_CODE_OK == DoStartElement()) {
		if (msg.result) {
			msg.result->set_value(ERR_CODE_OK);
		}	
	}
	else {
		if (msg.result) {
			msg.result->set_value(ERR_CODE_FAILED);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ElementBase::OnStopElementMsg(const CommonMsg& msg)
{
	if (!msg.dst.empty() && msg.dst != m_ele_name) {
		return;
	}

	LOG_INF("OnStopElementMsg, element:{}", m_ele_name);

	if (ERR_CODE_OK == DoStopElement()) {
		if (msg.result) {
			msg.result->set_value(ERR_CODE_OK);
		}
	}
	else {
		if (msg.result) {
			msg.result->set_value(ERR_CODE_FAILED);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ElementBase::OnPauseElementMsg(const CommonMsg& msg)
{
	if (!msg.dst.empty() && msg.dst != m_ele_name) {
		return;
	}

	LOG_INF("OnPauseElementMsg, element:{}", m_ele_name);

	if (ERR_CODE_OK == DoPauseElement()) {
		if (msg.result) {
			msg.result->set_value(ERR_CODE_OK);
		}
	}
	else {
		if (msg.result) {
			msg.result->set_value(ERR_CODE_FAILED);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ElementBase::OnResumeElementMsg(const CommonMsg& msg)
{
	if (!msg.dst.empty() && msg.dst != m_ele_name) {
		return;
	}

	LOG_INF("OnResumeElementMsg, element:{}", m_ele_name);

	if (ERR_CODE_OK == DoResumeElement()) {
		if (msg.result) {
			msg.result->set_value(ERR_CODE_OK);
		}
	}
	else {
		if (msg.result) {
			msg.result->set_value(ERR_CODE_FAILED);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ElementBase::PreProcPipelineMsg(const CommonMsg& msg)
{
	return ERR_CODE_MSG_NO_PROC;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ElementBase::OnPipelineMsg(const CommonMsg& msg)
{
	if (!msg.dst.empty() && msg.dst != m_ele_name) {
		return;
	}

	ErrCode result = PreProcPipelineMsg(msg);
	if (ERR_CODE_MSG_NO_PROC != result) {
		if (ERR_CODE_OK != result) {
			LOG_WRN("Pretranslate pipeline msg:{} failed!", 
				PIN_MSG_STR((stmr::PinMsgType)msg.msg_type));
		}
		return;
	}

	switch (msg.msg_type) {
	case stmr::PlMsgType::START_ELEMENT:
		OnStartElementMsg(msg);
		break;
	case stmr::PlMsgType::STOP_ELEMENT:
		OnStopElementMsg(msg);
		break;
	case stmr::PlMsgType::PAUSE_ELEMENT:
		OnPauseElementMsg(msg);
		break;
	case stmr::PlMsgType::RESUME_ELEMENT:
		OnResumeElementMsg(msg);
		break;
	default:
		LOG_WRN("Unknown pipeline msg:{}", 
			PIN_MSG_STR((stmr::PinMsgType)msg.msg_type));
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ElementBase::NotifyRunState(const std::string& desc)
{
	jukey::com::CommonMsg msg;
	msg.msg_type = (uint32_t)stmr::PlMsgType::RUN_STATE;
	msg.msg_data.reset(new std::string(desc));

	m_pipeline->PostPlMsg(msg);
}

}