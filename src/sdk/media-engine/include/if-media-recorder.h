#pragma once

#include "common-struct.h"
#include "common-error.h"

namespace jukey::sdk
{

//==============================================================================
// 
//==============================================================================
struct RecordParam
{
	std::string record_file;
};

//==============================================================================
// 
//==============================================================================
enum class RecordState
{
	RECORD_STATE_INVALID   = 0,
	RECORD_STATE_INITED    = 1,
	RECORD_STATE_RUNNING   = 2,
	RECORD_STATE_PAUSED    = 3,
	RECORD_STATE_STOPED    = 4,
};

//==============================================================================
// 
//==============================================================================
class IMediaRecorder
{
public:
	virtual com::ErrCode Init(const RecordParam& param) = 0;

	virtual com::ErrCode Start() = 0;
	virtual com::ErrCode Pause() = 0;
	virtual com::ErrCode Resume() = 0;
	virtual com::ErrCode Stop() = 0;

	virtual com::ErrCode AddStream(const com::Stream& stream) = 0;
	virtual com::ErrCode RemoveStream(const com::Stream& stream) = 0;

	virtual RecordState State() = 0;
};

}