#pragma once

#include "if-pipeline-processor.h"

namespace jukey::sdk
{

//==============================================================================
// 
//==============================================================================
class IPipelineProcessorHandler
{
public:
	virtual void OnAddStream(const com::ElementStream& stream,
		IPipelineProcessorSP processor) = 0;

	virtual void OnRemoveStream(const com::ElementStream& stream,
		IPipelineProcessorSP processor) = 0;

	virtual void OnRunState(const std::string& desc) = 0;

	virtual void OnPlayProgress(const com::MediaSrc& media_src, 
		uint32_t progress) = 0;

	virtual void OnAudioStreamStats(const com::MediaStream& stream, 
		const com::AudioStreamStats& stats) = 0;

	virtual void OnVideoStreamStats(const com::MediaStream& stream,
		const com::VideoStreamStats& stats) = 0;
};

//==============================================================================
// 
//==============================================================================
class PipelineProcessorBase 
	: public IPipelineProcessor
	, public stmr::IPlMsgHandler
	, public std::enable_shared_from_this<PipelineProcessorBase>
{
public:
	PipelineProcessorBase(base::IComFactory* factory, 
		PipelineProcessorMainType ppmt,
		PipelineProcessorSubType ppst,
		IPipelineProcessorHandler* handler);
	virtual ~PipelineProcessorBase();

	com::ErrCode Init();

	// IPipelineProcessor
	virtual com::ErrCode Start() override;
	virtual com::ErrCode Pause() override;
	virtual com::ErrCode Resume() override;
	virtual com::ErrCode Stop() override;
	virtual ProcessorState State() override;
	virtual PipelineProcessorMainType MainType() override;
	virtual PipelineProcessorSubType SubType() override;
	virtual stmr::IPipeline* Pipeline() override;
	virtual bool AllocSinkPin(const std::string& stream_id, 
		com::ElementPin& pin) override { return false; }
	virtual void ReleaseSinkPin(const com::ElementPin& pin) override {}
	virtual bool ContainStream(const std::string& stream_id) override { return false; }

	// IPlMsgHandler
	virtual void OnPipelineMsg(const com::CommonMsg& msg) override;

private:
	virtual com::ErrCode DoStart() { return com::ERR_CODE_OK; }
	virtual com::ErrCode DoStop() { return com::ERR_CODE_OK; }

	void OnAddElementStream(const com::CommonMsg& msg);
	void OnPlayProgress(const com::CommonMsg& msg);
	void OnRunState(const com::CommonMsg& msg);
	void OnAudioStreamStats(const com::CommonMsg& msg);
	void OnVideoStreamStats(const com::CommonMsg& msg);

protected:
	PipelineProcessorMainType m_ppmt = PPMT_INVALID;
	PipelineProcessorSubType m_ppst = PPST_INVALID;
	base::IComFactory* m_factory = nullptr;
	stmr::IPipeline* m_pipeline = nullptr;
	IPipelineProcessorHandler* m_handler = nullptr;
	ProcessorState m_state = ProcessorState::INVALID;
};

}