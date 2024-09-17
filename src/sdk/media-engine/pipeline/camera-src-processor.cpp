#include "camera-src-processor.h"
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
CameraSrcProcessor::CameraSrcProcessor(PipelineProcessorMgr* mgr)
	: PipelineProcessorBase(mgr->m_factory, PPMT_SRC, PPST_CAMERA, mgr)
	, m_pl_proc_mgr(mgr)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
CameraSrcProcessor::~CameraSrcProcessor()
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode CameraSrcProcessor::Init(const com::CamParam& param)
{
	PipelineProcessorBase::Init();

	IPropertyUP prop = util::MakeProperty(m_factory, "media engine");
	prop->SetU32Value("device-id", param.dev_id);
	prop->SetU32Value("resolution", param.resolution);
	prop->SetU32Value("pixel-format", param.pixel_format);
	prop->SetU32Value("frame-rate", param.frame_rate);

	if (!m_pipeline->AddElement(CID_CAMERA, prop.get())) {
		LOG_ERR("Create camera element failed!");
		return com::ERR_CODE_FAILED;
	}

	m_param = param;

	LOG_INF("Init CameraSrcProcessor success");

	return com::ErrCode::ERR_CODE_OK;
}

}