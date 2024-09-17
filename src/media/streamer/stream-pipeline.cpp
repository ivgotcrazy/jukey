#include <future>
#include <stack>

#include "stream-pipeline.h"
#include "element-factory.h"
#include "common/util-string.h"
#include "cap-negotiate.h"
#include "streamer-common.h"
#include "com-factory.h"
#include "pipeline-msg.h"
#include "sync-manager.h"
#include "common/util-common.h"
#include "log.h"

using namespace std::placeholders;
using namespace jukey::com;

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
StreamPipeline::StreamPipeline(base::IComFactory* factory, const char* owner)
	: ProxyUnknown(nullptr)
  , ComObjTracer(factory, CID_PIPELINE, owner)
	, CommonThread(std::string("pipeline-").append(owner), true)
	, m_factory(factory)
	, m_sync_mgr(SyncManager::Instance())
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
StreamPipeline::~StreamPipeline()
{
	LOG_INF("Destruct pipeline:{}", m_pipeline_name);

	Stop();

	m_msg_bus.UnsubscribeMsg(PlMsgType::ADD_ELEMENT_STREAM, this);
	m_msg_bus.UnsubscribeMsg(PlMsgType::DEL_ELEMENT_STREAM, this);

	m_msg_bus.Stop();

	StopThread();

	for (auto item : m_elements) {
		item.second->Release();
	}
	m_elements.clear();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* StreamPipeline::CreateInstance(base::IComFactory* factory, 
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_PIPELINE) == 0) {
		return new StreamPipeline(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* StreamPipeline::NDQueryInterface(const char* riid)
{
	if (0 == strcmp(riid, IID_PIPELINE)) {
		return static_cast<IPipeline*>(this);
	}
	else {
		return ProxyUnknown::NDQueryInterface(riid);
	}
}

//------------------------------------------------------------------------------
// Pipeline aware message handle in pipeline thread
//------------------------------------------------------------------------------
void StreamPipeline::HandleMsgBusMsg(const CommonMsg& msg)
{
	PostMsg(msg);
}

//------------------------------------------------------------------------------
// Must be???
//------------------------------------------------------------------------------
bool StreamPipeline::HasSrcElement()
{
	for (auto& item : m_elements) {
		if (EleMainType::SRC == item.second->MainType()) {
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool StreamPipeline::DoStartElements()
{
	for (auto item : m_elements) {
		CommonMsg msg(PlMsgType::START_ELEMENT);
		msg.dst = item.second->Name();
		if (ERR_CODE_OK != SendPlMsg(msg)) {
			LOG_ERR("Start element:{} failed!", item.second->Name());
			return false;
		}
	}

	m_state = PipelineState::PIPELINE_STATE_RUNNING;

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool StreamPipeline::DoPauseElements()
{
	for (auto item : m_elements) {
		CommonMsg msg(PlMsgType::PAUSE_ELEMENT);
		msg.dst = item.second->Name();
		if (ERR_CODE_OK != SendPlMsg(msg)) {
			LOG_ERR("Pause element:{} failed!", item.second->Name());
			return false;
		}
	}

	m_state = PipelineState::PIPELINE_STATE_PAUSED;

	return true;
}


//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool StreamPipeline::DoResumeElements()
{
	for (auto item : m_elements) {
		CommonMsg msg(PlMsgType::RESUME_ELEMENT);
		msg.dst = item.second->Name();
		if (ERR_CODE_OK != SendPlMsg(msg)) {
			LOG_ERR("Resume element:{} failed!", item.second->Name());
			return false;
		}
	}

	m_state = PipelineState::PIPELINE_STATE_RUNNING;

	return true;
}


//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool StreamPipeline::DoStopElements()
{
	for (auto item : m_elements) {
		CommonMsg msg(PlMsgType::STOP_ELEMENT);
		msg.dst = item.second->Name();
		if (ERR_CODE_OK != SendPlMsg(msg)) {
			LOG_ERR("Stop element:{} failed!", item.second->Name());
			return false;
		}
	}

	m_state = PipelineState::PIPELINE_STATE_STOPED;

	return true;
}


//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamPipeline::OnAddElementStream(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(com::ElementStream);

	LOG_INF("OnAddElementStream, element:{}, pin:{}, stream:{}",
		data->pin.ele_name, data->pin.pin_name, data->stream.stream.stream_id);

	auto iter = m_streams.find(data->stream.stream.stream_id);
	if (iter != m_streams.end()) {
		iter->second.push_back(*data);
	}
	else {
		std::vector<com::ElementStream> s{ *data };
		m_streams.insert(std::make_pair(data->stream.stream.stream_id, s));
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamPipeline::OnRemoveElementStream(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(com::ElementStream);

	LOG_INF("OnRemoveElementStream, stream:{}", data->stream.stream.stream_id);

	auto iter = m_streams.find(data->stream.stream.stream_id);
	if (iter == m_streams.end()) {
		LOG_ERR("Cannot find stream:{}!", data->stream.stream.stream_id);
		return;
	}

	for (auto i = iter->second.begin(); i != iter->second.end(); i++) {
		if (i->pin.ele_name == data->pin.ele_name 
			&& i->pin.pin_name == data->pin.pin_name) {
			iter->second.erase(i);
			break;
		}
	}

	if (iter->second.empty()) {
		m_streams.erase(data->stream.stream.stream_id);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamPipeline::OnThreadMsg(const com::CommonMsg& msg)
{
	switch (msg.msg_type) {
	case ADD_ELEMENT_STREAM:
		return OnAddElementStream(msg);
	case DEL_ELEMENT_STREAM:
		return OnRemoveElementStream(msg);
	default:
		LOG_ERR("Unknown message type:{}", msg.msg_type);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamPipeline::DumpPipeline()
{
	if (m_elements.empty()) {
		LOG_WRN("Empty pipeline:{}!", m_pipeline_name);
		return;
	}

	if (!HasSrcElement()) {
		LOG_WRN("Pipeline:{} has not src element!", m_pipeline_name);
		return;
	}

	bool print_header = false;

	for (auto item : m_elements) {
		std::string link;
		std::vector<std::string> links;

		if (item.second->MainType() == EleMainType::SRC) {
			if (!print_header) {
				LOG_INF("Dump pipeline {}:", m_pipeline_name);
				print_header = true;
			}
			DumpElement(item.second, link, links);
		}

		for (const auto& item : links) {
			LOG_INF("\t{}", item);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamPipeline::Init(CSTREF pipeline_name)
{
	m_pipeline_name = pipeline_name + std::to_string((uint64_t)this);

	StartThread();

	m_msg_bus.Start();

	m_msg_bus.SubscribeMsg(PlMsgType::ADD_ELEMENT_STREAM,
		std::bind(&StreamPipeline::HandleMsgBusMsg, this, _1), this);
	m_msg_bus.SubscribeMsg(PlMsgType::DEL_ELEMENT_STREAM,
		std::bind(&StreamPipeline::HandleMsgBusMsg, this, _1), this);

	m_state = PipelineState::PIPELINE_STATE_INITED;

	LOG_INF("Init pipeline:{} success", m_pipeline_name);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ISyncMgr& StreamPipeline::GetSyncMgr()
{
	return m_sync_mgr;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string StreamPipeline::Name()
{
	return m_pipeline_name;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamPipeline::Start()
{
	LOG_INF("[{}] Start", m_pipeline_name);

	return this->ExecuteSync([this](util::CallParam) -> com::ErrCode {
		if (m_state == PipelineState::PIPELINE_STATE_RUNNING) {
			LOG_ERR("Pipeline is already running!");
			return ERR_CODE_OK;
		}

		if (m_state != PipelineState::PIPELINE_STATE_INITED) {
			LOG_ERR("Invalid state:{} to start pipeline!", m_state);
			return ERR_CODE_FAILED;
		}

		if (!DoStartElements()) {
			LOG_ERR("DoStart failed!");
			return ERR_CODE_FAILED;
		}

		DumpPipeline();

		return ERR_CODE_OK;
	}, nullptr);	
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamPipeline::Pause()
{
	LOG_INF("[{}] Pause", m_pipeline_name);

	return this->ExecuteSync([this](util::CallParam) -> com::ErrCode {
		if (m_state == PipelineState::PIPELINE_STATE_PAUSED) {
			LOG_ERR("Pipeline is already paused!");
			return ERR_CODE_OK;
		}

		if (m_state != PipelineState::PIPELINE_STATE_RUNNING) {
			LOG_ERR("Invalid state:{} to pause pipeline!", m_state);
			return ERR_CODE_FAILED;
		}

		if (!DoPauseElements()) {
			LOG_ERR("DoPause failed!");
			return ERR_CODE_FAILED;
		}

		return ERR_CODE_OK;
	}, nullptr);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamPipeline::Resume()
{
	LOG_INF("[{}] Resume", m_pipeline_name);

	return this->ExecuteSync([this](util::CallParam) -> com::ErrCode {
		if (m_state == PipelineState::PIPELINE_STATE_RUNNING) {
			LOG_ERR("Pipeline is already running!");
			return ERR_CODE_OK;
		}

		if (m_state != PipelineState::PIPELINE_STATE_PAUSED) {
			LOG_ERR("Invalid state:{} to resume pipeline!", m_state);
			return ERR_CODE_FAILED;
		}

		if (!DoResumeElements()) {
			LOG_ERR("DoResume failed!");
			return ERR_CODE_FAILED;
		}

		DumpPipeline();

		return ERR_CODE_OK;
	}, nullptr);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamPipeline::Stop()
{
	LOG_INF("[{}] Stop", m_pipeline_name);

	return this->ExecuteSync([this](util::CallParam) -> com::ErrCode {
		if (m_state == PipelineState::PIPELINE_STATE_STOPED)
			return ERR_CODE_OK;

		if (!DoStopElements()) {
			LOG_ERR("DoStop failed!");
			return ERR_CODE_FAILED;
		}

		return ERR_CODE_OK;
	}, nullptr);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
PipelineState StreamPipeline::State()
{
	return m_state;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
IElement* StreamPipeline::AddElement(CSTREF cid, com::IProperty* props)
{
	LOG_INF("[{}] Add element, cid:{}", m_pipeline_name, cid);

	void* ele = this->ExecuteSync([this, cid, props](util::CallParam) -> void* {
		// Only can add element on INITED and PAUSED states
		if (m_state != PipelineState::PIPELINE_STATE_INITED &&
			m_state != PipelineState::PIPELINE_STATE_PAUSED) {
			LOG_ERR("Invalid pipeline state:{}", m_state);
			return nullptr;
		}

		IElement* ele = (IElement*)QI(cid.c_str(), IID_ELEMENT, m_pipeline_name);
		if (!ele) {
			LOG_ERR("Create element {} failed!", cid);
			return nullptr;
		}

		if (com::ERR_CODE_OK != ele->Init(this, props)) {
			LOG_ERR("Init element {} failed!", ele->Name());
			return nullptr;
		}

		auto result = m_elements.insert(std::make_pair(ele->Name(), ele));
		if (result.second) {
			LOG_INF("Add element success, name:{}", ele->Name());
			return ele;
		}
		else {
			LOG_INF("Add element failed, name:{}", ele->Name());
			ele->Release();
			return nullptr;
		}

		return ele;
	}, nullptr);

	return (IElement*)ele;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
IElement* StreamPipeline::GetElementByName(CSTREF name)
{
	void* ele = this->ExecuteSync([this, name](util::CallParam) -> void* {
		auto iter = m_elements.find(name);
		if (iter == m_elements.end()) {
			LOG_ERR("Cannot find element:{} in pipeline:{}", name, m_pipeline_name);
			return nullptr;
		}
		else {
			return iter->second;
		}
	}, nullptr);

	return (IElement*)ele;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamPipeline::RemoveElement(CSTREF name)
{
	return this->ExecuteSync([this, name](util::CallParam) -> com::ErrCode {
		size_t count = m_elements.erase(name);
		if (count == 0) {
			LOG_ERR("Element does not exist!");
			return ERR_CODE_FAILED;
		}
		else {
			return ERR_CODE_OK;
		}
	}, nullptr);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamPipeline::PostPlMsg(const com::CommonMsg& msg)
{
	m_msg_bus.PublishMsg(msg);
}

//------------------------------------------------------------------------------
// TODO: be care of dead lock!!!
//------------------------------------------------------------------------------
com::ErrCode StreamPipeline::SendPlMsg(const com::CommonMsg& msg)
{
	com::CommonMsg& ref = const_cast<com::CommonMsg&>(msg);

	ref.result.reset(new std::promise<com::ErrCode>());
	
	m_msg_bus.PublishMsg(ref);

	return ref.result->get_future().get();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamPipeline::SubscribeMsg(uint32_t msg_type, IPlMsgHandler* handler)
{
	if (!handler) {
		LOG_ERR("Invalid pipeline message handler!");
		return ERR_CODE_INVALID_PARAM;
	}
	
	m_msg_bus.SubscribeMsg(msg_type, 
		std::bind(&IPlMsgHandler::OnPipelineMsg, handler, _1), handler);

	return com::ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamPipeline::UnsubscribeMsg(uint32_t msg_type, IPlMsgHandler* handler)
{
	if (!handler) {
		LOG_ERR("Invalid pipeline message handler!");
		return ERR_CODE_INVALID_PARAM;
	}

	m_msg_bus.UnsubscribeMsg(msg_type, handler);

	return com::ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamPipeline::LinkElement(ISrcPin* src_pin, ISinkPin* sink_pin)
{
	if (!src_pin) {
		LOG_ERR("Invalid src pin!");
		return ERR_CODE_INVALID_PARAM;
	}

	if (!sink_pin) {
		LOG_ERR("Invalid sink pin!");
		return ERR_CODE_INVALID_PARAM;
	}

	LOG_INF("Link element, src pin:{}, sink pin:{}", src_pin->ToStr(),
		sink_pin->ToStr());

	return this->ExecuteSync([this, src_pin, sink_pin](util::CallParam) -> com::ErrCode {
		// Only can link element on INITED and PAUSED states
		if (m_state != PipelineState::PIPELINE_STATE_INITED
			&& m_state != PipelineState::PIPELINE_STATE_PAUSED) {
			LOG_ERR("Invalid pipeline state:{}", m_state);
			return ERR_CODE_FAILED;
		}

		if (ERR_CODE_OK != src_pin->AddSinkPin(sink_pin)) {
			LOG_ERR("Add sink pin failed, sink pin:{}", sink_pin->ToStr());
			return ERR_CODE_FAILED;
		}

		return ERR_CODE_OK;
	}, nullptr);
}

}