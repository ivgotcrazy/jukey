#pragma once

#include "common-struct.h"
#include "pipeline-processor-base.h"

namespace jukey::sdk
{

class PipelineProcessorMgr;

//==============================================================================
// 
//==============================================================================
class MediaFileProcessor : public PipelineProcessorBase
{
public:
	MediaFileProcessor(PipelineProcessorMgr* mgr);
	~MediaFileProcessor();

	com::ErrCode Init(const std::string& file);

	const std::string& GetFile() { return m_file; }

private:
	PipelineProcessorMgr* m_pl_proc_mgr = nullptr;
	std::string m_file;
};
typedef std::shared_ptr<MediaFileProcessor> MediaFileProcessorSP;

}