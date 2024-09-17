#include "raw-audio-send-processor.h"
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
RawAudioSendProcessor::RawAudioSendProcessor(PipelineProcessorMgr* mgr)
	: PipelineProcessorBase(mgr->m_factory, PPMT_PROCESSOR, PPST_AUDIO_RAW_SENDER, 
		mgr)
	, m_pl_proc_mgr(mgr)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
RawAudioSendProcessor::~RawAudioSendProcessor()
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode RawAudioSendProcessor::Init(const com::MediaStream& stream,
	const std::string& addr)
{
	PipelineProcessorBase::Init();

	// Converter
	m_convert_element = m_pipeline->AddElement(CID_AUDIO_CONVERT, nullptr);
	if (!m_convert_element) {
		LOG_ERR("Create audio convert element failed!");
		return ERR_CODE_FAILED;
	}

	// Encoder
	auto encode_element = m_pipeline->AddElement(CID_AUDIO_ENCODE, nullptr);
	if (!encode_element) {
		LOG_ERR("Create audio encode element failed!");
		return ERR_CODE_FAILED;
	}

	// Sender
	util::IPropertyUP prop = util::MakeProperty(m_factory, "processor");
	prop->SetPtrValue("session-mgr", m_pl_proc_mgr->m_sess_mgr);
	prop->SetStrValue("service-addr", addr.c_str());
	prop->SetPtrValue("media-stream", &stream);
	auto send_element = m_pipeline->AddElement(CID_AUDIO_SEND, prop.get());
	if (!send_element) {
		LOG_ERR("Create audio stream send element failed!");
		return ERR_CODE_FAILED;
	}

	// Encoder -> Sender
	if (ERR_CODE_OK != m_pipeline->LinkElement(encode_element->SrcPins().front(),
		send_element->SinkPins().front())) {
		LOG_ERR("Link encoder and sender failed!");
		return ERR_CODE_FAILED;
	}

	// Converter -> Encoder
	if (ERR_CODE_OK != m_pipeline->LinkElement(m_convert_element->SrcPins().front(),
		encode_element->SinkPins().front())) {
		LOG_ERR("Link converter and encoder failed!");
		return ERR_CODE_FAILED;
	}

	m_media_stream = stream;
	m_addr = addr;

	LOG_INF("Init RawAudioSendProcessor success");

	return com::ErrCode::ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool RawAudioSendProcessor::AllocSinkPin(const std::string& stream_id,
	com::ElementPin& pin)
{
	if (!m_convert_element) {
		LOG_ERR("Invalid decode element!");
		return false;
	}

	if (m_convert_element->SinkPins().empty()) {
		LOG_ERR("Empty sink pins!");
		return false;
	}

	pin.pl_name = m_pipeline->Name();
	pin.ele_name = m_convert_element->Name();
	pin.pin_name = m_convert_element->SinkPins().front()->Name();

	LOG_INF("Alloc sink pin, pl:{}, ele:{}, pin:{}", pin.pl_name,
		pin.ele_name, pin.pin_name);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RawAudioSendProcessor::ReleaseSinkPin(const com::ElementPin& pin)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool RawAudioSendProcessor::ContainStream(const std::string& stream_id)
{
	return stream_id == m_media_stream.stream.stream_id;
}

}