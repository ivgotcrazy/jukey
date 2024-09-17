#pragma once

#include "common-struct.h"
#include "pipeline-processor-base.h"

namespace jukey::sdk
{

class PipelineProcessorMgr;

//==============================================================================
// 
//==============================================================================
class CameraSrcProcessor : public PipelineProcessorBase
{
public:
	CameraSrcProcessor(PipelineProcessorMgr* mgr);
	~CameraSrcProcessor();

	com::ErrCode Init(const com::CamParam& param);

	const com::CamParam& GetCamParam() { return m_param; }

private:
	PipelineProcessorMgr* m_pl_proc_mgr = nullptr;
	com::CamParam m_param;
};
typedef std::shared_ptr<CameraSrcProcessor> CameraSrcProcessorSP;

}