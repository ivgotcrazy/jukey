#pragma once

#include <memory>

#include "common-struct.h"
#include "if-pipeline.h"
#include "if-property.h"
#include "engine-common.h"


namespace jukey::sdk
{

//==============================================================================
// 
//==============================================================================
class IPipelineProcessor
{
public:
	virtual ~IPipelineProcessor() {}
	
	//
	// Start stream processor
	//
	virtual com::ErrCode Start() = 0;

	//
	// Pause stream procesor while in start state
	//
	virtual com::ErrCode Pause() = 0;

	//
	// Resume stream processor while in pause state
	//
	virtual com::ErrCode Resume() = 0;

	//
	// Stop stream processor
	//
	virtual com::ErrCode Stop() = 0;

	// 
	// Get stream processor state
	//
	virtual ProcessorState State() = 0;

	//
	// Get stream processor main type
	//
	virtual PipelineProcessorMainType MainType() = 0;

	//
	// Get stream processor sub-type
	//
	virtual PipelineProcessorSubType SubType() = 0;

	//
	// Get stream processor pipeline, one processor has one pipeline generally
	//
	virtual stmr::IPipeline* Pipeline() = 0;

	//
	// Allocate sink pin for stream
	//
	virtual bool AllocSinkPin(const std::string& stream_id, 
		com::ElementPin& pin) = 0;

	//
	// Release sink pin
	//
	virtual void ReleaseSinkPin(const com::ElementPin& pin) = 0;

	//
	// Contain stream
	//
	virtual bool ContainStream(const std::string& stream_id) = 0;
};
typedef std::shared_ptr<IPipelineProcessor> IPipelineProcessorSP;

}