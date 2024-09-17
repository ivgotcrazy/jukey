#include "raw-video-render-processor.h"
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
RawVideoRenderProcessor::RawVideoRenderProcessor(PipelineProcessorMgr* mgr)
	: PipelineProcessorBase(mgr->m_factory, PPMT_PROCESSOR, PPST_VIDEO_RAW_RENDER,
			mgr)
	, m_pl_proc_mgr(mgr)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
RawVideoRenderProcessor::~RawVideoRenderProcessor()
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode RawVideoRenderProcessor::Init(const com::MediaStream& stream,
	void* wnd)
{
	PipelineProcessorBase::Init();

	// Convert
	m_convert_element = m_pipeline->AddElement(CID_VIDEO_CONVERT, nullptr);
	if (!m_convert_element) {
		LOG_ERR("Create video convert element failed!");
		return ERR_CODE_FAILED;
	}

	// Render
	IPropertyUP prop = util::MakeProperty(m_factory, "media engine");
	prop->SetPtrValue("render-wnd", wnd);
	prop->SetPtrValue("executor", m_pl_proc_mgr->m_executor);
	auto render_element = m_pipeline->AddElement(CID_VIDEO_RENDER, prop.get());
	if (!render_element) {
		LOG_ERR("Create video render element failed!");
		return ERR_CODE_FAILED;
	}

	// Converter -> Renderer
	if (ERR_CODE_OK != m_pipeline->LinkElement(m_convert_element->SrcPins().front(),
		render_element->SinkPins().front())) {
		LOG_ERR("Link converter and renderer failed!");
		return ERR_CODE_FAILED;
	}

	LOG_INF("Init RawVideoRenderProcessor success");

	return com::ErrCode::ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool RawVideoRenderProcessor::AllocSinkPin(const std::string& stream_id,
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
void RawVideoRenderProcessor::ReleaseSinkPin(const com::ElementPin& pin)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool RawVideoRenderProcessor::ContainStream(const std::string& stream_id)
{
	return stream_id == m_stream_id;
}

}