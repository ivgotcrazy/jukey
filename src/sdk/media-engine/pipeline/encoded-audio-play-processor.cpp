#include "encoded-audio-play-processor.h"
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
EncodedAudioPlayProcessor::EncodedAudioPlayProcessor(PipelineProcessorMgr* mgr)
	: PipelineProcessorBase(mgr->m_factory, PPMT_PROCESSOR, 
		PPST_AUDIO_ENCODED_PLAYER, mgr)
	, m_pl_proc_mgr(mgr)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
EncodedAudioPlayProcessor::~EncodedAudioPlayProcessor()
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode EncodedAudioPlayProcessor::Init(const com::MediaStream& stream,
	bool sync)
{
	PipelineProcessorBase::Init();

	// Decoder
	m_decode_element = m_pipeline->AddElement(CID_AUDIO_DECODE, nullptr);
	if (!m_decode_element) {
		LOG_ERR("Create audio decode element failed!");
		return ERR_CODE_FAILED;
	}

	// Convert
	auto convert_element = m_pipeline->AddElement(CID_AUDIO_CONVERT, nullptr);
	if (!convert_element) {
		LOG_ERR("Create audio convert element failed!");
		return ERR_CODE_FAILED;
	}

	// Play
	IPropertyUP prop = util::MakeProperty(m_factory, "processor");
	prop->SetU32Value("mode", sync ? 1 : 0);
	auto play_element = m_pipeline->AddElement(CID_AUDIO_PLAY, prop.get());
	if (!play_element) {
		LOG_ERR("Create audio play element failed!");
		return ERR_CODE_FAILED;
	}

	// Converter -> Play
	if (ERR_CODE_OK != m_pipeline->LinkElement(convert_element->SrcPins().front(),
		play_element->SinkPins().front())) {
		LOG_ERR("Link convert and play failed!");
		return ERR_CODE_FAILED;
	}

	// Decoder -> Converter
	if (ERR_CODE_OK != m_pipeline->LinkElement(m_decode_element->SrcPins().front(),
		convert_element->SinkPins().front())) {
		LOG_ERR("Link decoder and converter failed!");
		return ERR_CODE_FAILED;
	}

	LOG_INF("Init EncodedAudioPlayProcessor success");

	return com::ErrCode::ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool EncodedAudioPlayProcessor::AllocSinkPin(const std::string& stream_id, 
	com::ElementPin& pin)
{
	if (!m_decode_element) {
		LOG_ERR("Invalid decode element!");
		return false;
	}

	if (m_decode_element->SinkPins().empty()) {
		LOG_ERR("Empty sink pins!");
		return false;
	}

	pin.pl_name = m_pipeline->Name();
	pin.ele_name = m_decode_element->Name();
	pin.pin_name = m_decode_element->SinkPins().front()->Name();

	m_stream_id = stream_id;
	m_pin = pin;

	LOG_INF("Alloc sink pin, pl:{}, ele:{}, pin:{}", pin.pl_name,
		pin.ele_name, pin.pin_name);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void EncodedAudioPlayProcessor::ReleaseSinkPin(const com::ElementPin& pin)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool EncodedAudioPlayProcessor::ContainStream(const std::string& stream_id)
{
	return stream_id == m_stream_id;
}

}