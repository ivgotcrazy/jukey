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
class EncodedAudioSendProcessor : public PipelineProcessorBase
{
public:
	EncodedAudioSendProcessor(PipelineProcessorMgr* mgr);
	~EncodedAudioSendProcessor();

	com::ErrCode Init(const com::MediaStream& stream, const std::string& addr);

	// IPipelineProcessor
	virtual bool AllocSinkPin(const std::string& stream_id, 
		com::ElementPin& pin) override;
	virtual void ReleaseSinkPin(const com::ElementPin& pin) override;
	virtual bool ContainStream(const std::string& stream_id) override;

private:
	PipelineProcessorMgr* m_pl_proc_mgr = nullptr;
	com::MediaStream m_media_stream;
	std::string m_addr;
	stmr::IElement* m_decode_element = nullptr;
};
typedef std::shared_ptr<EncodedAudioSendProcessor> EncodedAudioSendProcessorSP;

}