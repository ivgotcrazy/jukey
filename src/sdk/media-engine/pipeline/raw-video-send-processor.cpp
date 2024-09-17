#include "raw-video-send-processor.h"
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
RawVideoSendProcessor::RawVideoSendProcessor(PipelineProcessorMgr* mgr)
	: PipelineProcessorBase(mgr->m_factory, PPMT_PROCESSOR, PPST_VIDEO_RAW_SENDER,
			mgr)
	, m_pl_proc_mgr(mgr)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
RawVideoSendProcessor::~RawVideoSendProcessor()
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode RawVideoSendProcessor::Init(const com::MediaStream& stream,
	const std::string& addr)
{
	PipelineProcessorBase::Init();

	// Converter
	m_convert_element = m_pipeline->AddElement(CID_VIDEO_CONVERT, nullptr);
	if (!m_convert_element) {
		LOG_ERR("Create video convert element failed!");
		return ERR_CODE_FAILED;
	}

	// Encoder
	util::IPropertyUP p1 = util::MakeProperty(m_factory, "");
	p1->SetPtrValue("bitrate-allocate-mgr", m_pl_proc_mgr->m_br_alloc_mgr);
	auto encode_element = m_pipeline->AddElement(CID_VIDEO_ENCODE, p1.get());
	if (!encode_element) {
		LOG_ERR("Create video encode element failed!");
		return ERR_CODE_FAILED;
	}

	// Sender
	util::IPropertyUP p2 = util::MakeProperty(m_factory, "processor");
	p2->SetPtrValue("session-mgr", m_pl_proc_mgr->m_sess_mgr);
	p2->SetStrValue("service-addr", addr.c_str());
	p2->SetPtrValue("media-stream", &stream);
	p2->SetPtrValue("bitrate-allocate-mgr", m_pl_proc_mgr->m_br_alloc_mgr);
	auto send_element = m_pipeline->AddElement(CID_VIDEO_SEND, p2.get());
	if (!send_element) {
		LOG_ERR("Create video stream send element failed!");
		return ERR_CODE_FAILED;
	}

	// Encoder -> Sender
	if (ERR_CODE_OK != m_pipeline->LinkElement(encode_element->SrcPins().front(),
		send_element->SinkPins().front())) {
		LOG_ERR("Link encoder and sender failed!");
		return ERR_CODE_FAILED;
	}

	// Converter -> Encoder
	if (ERR_CODE_OK != m_pipeline->LinkElement(
		m_convert_element->SrcPins().front(),
		encode_element->SinkPins().front())) {
		LOG_ERR("Link converter and encoder failed!");
		return ERR_CODE_FAILED;
	}

	m_media_stream = stream;
	m_addr = addr;

	LOG_INF("Init RawVideoSendProcessor success");

	return com::ErrCode::ERR_CODE_OK;
}


//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool RawVideoSendProcessor::AllocSinkPin(const std::string& stream_id,
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
void RawVideoSendProcessor::ReleaseSinkPin(const com::ElementPin& pin)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool RawVideoSendProcessor::ContainStream(const std::string& stream_id)
{
	return stream_id == m_media_stream.stream.stream_id;
}

}