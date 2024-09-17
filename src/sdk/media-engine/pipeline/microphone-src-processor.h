#pragma once

#include "common-struct.h"
#include "pipeline-processor-base.h"

namespace jukey::sdk
{

class PipelineProcessorMgr;

//==============================================================================
// 
//==============================================================================
class MicrophoneSrcProcessor : public PipelineProcessorBase
{
public:
	MicrophoneSrcProcessor(PipelineProcessorMgr* mgr);
	~MicrophoneSrcProcessor();

	com::ErrCode Init(const com::MicParam& param);

	const com::MicParam& GetMicParam() { return m_param; }

private:
	PipelineProcessorMgr* m_pl_proc_mgr = nullptr;
	com::MicParam m_param;
};
typedef std::shared_ptr<MicrophoneSrcProcessor> MicrophoneSrcProcessorSP;

}