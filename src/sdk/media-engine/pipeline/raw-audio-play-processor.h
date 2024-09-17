#pragma once

#include "common-struct.h"
#include "pipeline-processor-base.h"

namespace jukey::sdk
{

class PipelineProcessorMgr;

//==============================================================================
// 
//==============================================================================

class RawAudioPlayProcessor : public PipelineProcessorBase
{
public:
	RawAudioPlayProcessor(PipelineProcessorMgr* mgr);
	~RawAudioPlayProcessor();

	com::ErrCode Init(const com::MediaStream& stream);

	const com::MediaStream& GetMediaStream() { return m_media_stream; }

	// IPipelineProcessor
	virtual bool AllocSinkPin(const std::string& stream_id, 
		com::ElementPin& pin) override;
	virtual void ReleaseSinkPin(const com::ElementPin& pin) override;
	virtual bool ContainStream(const std::string& stream_id) override;

private:
	PipelineProcessorMgr* m_pl_proc_mgr = nullptr;
	com::MediaStream m_media_stream;
	stmr::IElement* m_convert_element = nullptr;
	std::string m_stream_id;
	com::ElementPin m_pin;
};
typedef std::shared_ptr<RawAudioPlayProcessor> RawAudioPlayProcessorSP;

}