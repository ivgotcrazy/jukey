#pragma once

#include "common-struct.h"
#include "pipeline-processor-base.h"
#include "if-session-mgr.h"

namespace jukey::sdk
{

class PipelineProcessorMgr;

//==============================================================================
// 
//==============================================================================
class AudioStreamSrcProcessor : public PipelineProcessorBase
{
public:
	AudioStreamSrcProcessor(PipelineProcessorMgr* mgr);
	~AudioStreamSrcProcessor();

	com::ErrCode Init(const com::MediaStream& stream, const std::string& addr);

	const com::MediaStream& GetMediaStream() { return m_media_stream; }
	const std::string& GetAddr() { return m_addr; }

private:
	PipelineProcessorMgr* m_pl_proc_mgr = nullptr;
	com::MediaStream m_media_stream;
	std::string m_addr;
};
typedef std::shared_ptr<AudioStreamSrcProcessor> AudioStreamSrcProcessorSP;

}