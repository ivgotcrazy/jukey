#include "media-file-processor.h"
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
MediaFileProcessor::MediaFileProcessor(PipelineProcessorMgr* mgr)
	: PipelineProcessorBase(mgr->m_factory, PPMT_SRC, PPST_MEDIA_FILE, mgr)
	, m_pl_proc_mgr(mgr)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MediaFileProcessor::~MediaFileProcessor()
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode MediaFileProcessor::Init(const std::string& file)
{
	PipelineProcessorBase::Init();

	IPropertyUP prop = util::MakeProperty(m_factory, "");
	prop->SetStrValue("file-path", file.c_str());

	if (!m_pipeline->AddElement(CID_FILE_DEMUX, prop.get())) {
		LOG_ERR("Create media file element failed!");
		return com::ERR_CODE_FAILED;
	}

	m_file = file;

	LOG_INF("Init MediaFileProcessor success");

	return com::ErrCode::ERR_CODE_OK;
}

}