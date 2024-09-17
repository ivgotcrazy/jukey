#include "video-stream-src-processor.h"
#include "if-element.h"
#include "common/util-property.h"
#include "log.h"
#include "pipeline-processor-mgr.h"

using namespace jukey::util;

namespace jukey::sdk
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
VideoStreamSrcProcessor::VideoStreamSrcProcessor(PipelineProcessorMgr* mgr)
	: PipelineProcessorBase(mgr->m_factory, PPMT_SRC, PPST_VIDEO_STREAM_SRC, mgr)
	, m_pl_proc_mgr(mgr)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
VideoStreamSrcProcessor::~VideoStreamSrcProcessor()
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode VideoStreamSrcProcessor::Init(const com::MediaStream& stream,
	const std::string& addr)
{
	PipelineProcessorBase::Init();

	util::IPropertyUP prop = util::MakeProperty(m_factory, "processor");
	prop->SetPtrValue("session-mgr", m_pl_proc_mgr->m_sess_mgr);
	prop->SetStrValue("service-addr", addr.c_str());
	prop->SetPtrValue("media-stream", &stream);

	if (!m_pipeline->AddElement(CID_VIDEO_RECV, prop.get())) {
		LOG_ERR("Create video recv stream element failed!");
		return com::ERR_CODE_FAILED;
	}

	m_media_stream = stream;
	m_addr = addr;

	LOG_INF("Init VideoStreamSrcProcessor success");

	return com::ErrCode::ERR_CODE_OK;
}

}