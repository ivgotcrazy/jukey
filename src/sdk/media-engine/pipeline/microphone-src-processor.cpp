#include "microphone-src-processor.h"
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
MicrophoneSrcProcessor::MicrophoneSrcProcessor(PipelineProcessorMgr* mgr)
	: PipelineProcessorBase(mgr->m_factory, PPMT_SRC, PPST_MICROPHONE, mgr)
	, m_pl_proc_mgr(mgr)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MicrophoneSrcProcessor::~MicrophoneSrcProcessor()
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode MicrophoneSrcProcessor::Init(const com::MicParam& param)
{
	PipelineProcessorBase::Init();

	IPropertyUP prop = util::MakeProperty(m_factory, "");
	prop->SetU32Value("device-id", param.dev_id);
	prop->SetU32Value("sample-chnl", param.sample_chnl);
	prop->SetU32Value("sample-rate", param.sample_rate);
	prop->SetU32Value("sample-bits", param.sample_bits);

	if (!m_pipeline->AddElement(CID_MICROPHONE, prop.get())) {
		LOG_ERR("Create microphone element failed!");
		return com::ERR_CODE_FAILED;
	}

	m_param = param;

	LOG_INF("Init MicrophoneSrcProcessor success");

	return com::ErrCode::ERR_CODE_OK;
}

}