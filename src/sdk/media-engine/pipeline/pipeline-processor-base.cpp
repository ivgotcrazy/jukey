#include "pipeline-processor-base.h"
#include "log.h"
#include "pipeline-msg.h"

using namespace jukey::com;

namespace jukey::sdk
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
PipelineProcessorBase::PipelineProcessorBase(base::IComFactory* factory,
	PipelineProcessorMainType ppmt, 
	PipelineProcessorSubType ppst, 
	IPipelineProcessorHandler* handler)
	: m_factory(factory)
	, m_ppmt(ppmt)
	, m_ppst(ppst)
	, m_handler(handler)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
PipelineProcessorBase::~PipelineProcessorBase()
{
	if (m_pipeline) {
		m_pipeline->Stop();

		m_pipeline->UnsubscribeMsg(stmr::PlMsgType::ADD_ELEMENT_STREAM, this);
		m_pipeline->UnsubscribeMsg(stmr::PlMsgType::DEL_ELEMENT_STREAM, this);
		m_pipeline->UnsubscribeMsg(stmr::PlMsgType::RUN_STATE, this);
		m_pipeline->UnsubscribeMsg(stmr::PlMsgType::AUIDO_STREAM_STATS, this);
		m_pipeline->UnsubscribeMsg(stmr::PlMsgType::VIDEO_STREAM_STATS, this);
		m_pipeline->UnsubscribeMsg(stmr::PlMsgType::PLAY_PROGRESS, this);

		m_pipeline->Release();
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode PipelineProcessorBase::Init()
{
	m_pipeline = (stmr::IPipeline*)QI(CID_PIPELINE, IID_PIPELINE, "");
	if (!m_pipeline) {
		LOG_ERR("Create pipeline failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != m_pipeline->Init("")) {
		LOG_ERR("Init pipeline failed!");
		m_pipeline->Release();
		return ERR_CODE_FAILED;
	}

	m_pipeline->SubscribeMsg(stmr::PlMsgType::ADD_ELEMENT_STREAM, this);
	m_pipeline->SubscribeMsg(stmr::PlMsgType::DEL_ELEMENT_STREAM, this);
	m_pipeline->SubscribeMsg(stmr::PlMsgType::RUN_STATE, this);
	m_pipeline->SubscribeMsg(stmr::PlMsgType::AUIDO_STREAM_STATS, this);
	m_pipeline->SubscribeMsg(stmr::PlMsgType::VIDEO_STREAM_STATS, this);
	m_pipeline->SubscribeMsg(stmr::PlMsgType::PLAY_PROGRESS, this);

	m_state = ProcessorState::INITED;

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode PipelineProcessorBase::Start()
{
	LOG_INF("Start stream processor base");

	if (m_state == ProcessorState::RUNNING) {
		LOG_WRN("Processor is already running!");
		return ERR_CODE_OK;
	}

	if (m_state != ProcessorState::INITED) {
		LOG_ERR("Invaid processor state:{}", m_state);
		return ERR_CODE_FAILED;
	}

	if (com::ERR_CODE_OK != m_pipeline->Start()) {
		LOG_ERR("Start pipeline failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != DoStart()) {
		LOG_ERR("DoStart failed!");
		return ERR_CODE_FAILED;
	}

	m_state = ProcessorState::RUNNING;

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode PipelineProcessorBase::Pause()
{
	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode PipelineProcessorBase::Resume()
{
	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode PipelineProcessorBase::Stop()
{
	if (com::ERR_CODE_OK != m_pipeline->Stop()) {
		LOG_ERR("Stop pipeline failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != DoStop()) {
		LOG_ERR("DoStart failed!");
		return ERR_CODE_FAILED;
	}

	m_state = ProcessorState::STOPED;

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ProcessorState PipelineProcessorBase::State()
{
	return m_state;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
PipelineProcessorMainType PipelineProcessorBase::MainType()
{
	return m_ppmt;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
PipelineProcessorSubType PipelineProcessorBase::SubType()
{
	return m_ppst;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
stmr::IPipeline* PipelineProcessorBase::Pipeline()
{
	return m_pipeline;
}

//------------------------------------------------------------------------------
// Source element notify media stream
//------------------------------------------------------------------------------
void PipelineProcessorBase::OnAddElementStream(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(com::ElementStream);

	LOG_INF("Notify add element stream, pin:{}|{}, media:{}|{}, stream:{}|{}",
		data->pin.ele_name,
		data->pin.pin_name,
		data->stream.src.src_type,
		data->stream.src.src_id,
		data->stream.stream.stream_type,
		data->stream.stream.stream_id);

	m_handler->OnAddStream(*data, shared_from_this());
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void PipelineProcessorBase::OnPlayProgress(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(stmr::PlayProgressData);
	m_handler->OnPlayProgress(data->msrc, data->progress);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void PipelineProcessorBase::OnRunState(const com::CommonMsg& msg)
{
	std::string desc = *std::static_pointer_cast<std::string>(msg.msg_data).get();
	m_handler->OnRunState(desc);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void PipelineProcessorBase::OnAudioStreamStats(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(stmr::AudioStreamStatsData);
	m_handler->OnAudioStreamStats(data->stream, data->stats);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void PipelineProcessorBase::OnVideoStreamStats(const com::CommonMsg& msg)
{
	PCAST_COMMON_MSG_DATA(stmr::VideoStreamStatsData);
	m_handler->OnVideoStreamStats(data->stream, data->stats);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void PipelineProcessorBase::OnPipelineMsg(const com::CommonMsg& msg)
{
	switch ((stmr::PlMsgType)msg.msg_type) {
	case stmr::PlMsgType::RUN_STATE:
		OnRunState(msg);
		break;
	case stmr::PlMsgType::AUIDO_STREAM_STATS:
		OnAudioStreamStats(msg);
		break;
	case stmr::PlMsgType::VIDEO_STREAM_STATS:
		OnVideoStreamStats(msg);
		break;
	case stmr::PlMsgType::ADD_ELEMENT_STREAM:
		OnAddElementStream(msg);
		break;
	case stmr::PlMsgType::DEL_ELEMENT_STREAM:
		break;
	case stmr::PlMsgType::PLAY_PROGRESS:
		OnPlayProgress(msg);
		break;
	default:
		LOG_ERR("Unknown pipeline message:{}", msg.msg_type);
	}
}

}