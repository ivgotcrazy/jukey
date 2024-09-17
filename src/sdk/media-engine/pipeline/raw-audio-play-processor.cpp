#include "raw-audio-play-processor.h"
#include "if-element.h"
#include "common/util-property.h"
#include "log.h"
#include "pipeline-processor-mgr.h"

using namespace jukey::com;
using namespace jukey::util;

namespace jukey::sdk
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
RawAudioPlayProcessor::RawAudioPlayProcessor(PipelineProcessorMgr* mgr)
	: PipelineProcessorBase(mgr->m_factory, PPMT_PROCESSOR, PPST_AUDIO_RAW_PLAYER, 
		mgr)
	, m_pl_proc_mgr(mgr)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
RawAudioPlayProcessor::~RawAudioPlayProcessor()
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode RawAudioPlayProcessor::Init(const com::MediaStream& stream)
{
	PipelineProcessorBase::Init();

	// Convert
	m_convert_element = m_pipeline->AddElement(CID_AUDIO_CONVERT, nullptr);
	if (!m_convert_element) {
		LOG_ERR("Create audio convert element failed!");
		return ERR_CODE_FAILED;
	}

	// Player
	auto play_element = m_pipeline->AddElement(CID_AUDIO_PLAY, nullptr);
	if (!play_element) {
		LOG_ERR("Create audio play element failed!");
		return ERR_CODE_FAILED;
	}

	// Converter -> Player
	if (ERR_CODE_OK != m_pipeline->LinkElement(m_convert_element->SrcPins().front(),
		play_element->SinkPins().front())) {
		LOG_ERR("Link converter and player failed!");
		return ERR_CODE_FAILED;
	}

	m_media_stream = stream;

	LOG_INF("Init RawAudioPlayProcessor success");

	return com::ErrCode::ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool RawAudioPlayProcessor::AllocSinkPin(const std::string& stream_id,
	com::ElementPin& pin)
{
	if (!m_convert_element) {
		LOG_ERR("Invalid convert element!");
		return false;
	}

	if (m_convert_element->SinkPins().empty()) {
		LOG_ERR("Empty sink pins!");
		return false;
	}

	pin.pl_name = m_pipeline->Name();
	pin.ele_name = m_convert_element->Name();
	pin.pin_name = m_convert_element->SinkPins().front()->Name();

	m_stream_id = stream_id;
	m_pin = pin;

	LOG_INF("Alloc sink pin, pl:{}, ele:{}, pin:{}", pin.pl_name,
		pin.ele_name, pin.pin_name);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RawAudioPlayProcessor::ReleaseSinkPin(const com::ElementPin& pin)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool RawAudioPlayProcessor::ContainStream(const std::string& stream_id)
{
	return stream_id == m_stream_id;
}

}