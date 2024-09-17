#pragma once

#include <string>

#include "if-pin.h"
#include "common-message.h"

namespace jukey::sdk
{

//==============================================================================
// 
//==============================================================================
enum PipelineProcessorMainType
{
	PPMT_INVALID = 0,
	PPMT_SRC,
	PPMT_PROCESSOR,
};

//==============================================================================
// 
//==============================================================================
enum PipelineProcessorSubType
{
	PPST_INVALID = 0,
	PPST_CAMERA,
	PPST_MICROPHONE,
	PPST_MEDIA_FILE,
	PPST_AUDIO_STREAM_SRC,
	PPST_VIDEO_STREAM_SRC,
	PPST_VIDEO_RAW_RENDER,
	PPST_VIDEO_ENCODED_RENDER,
	PPST_VIDEO_RAW_SENDER,
	PPST_VIDEO_ENCODED_SENDER,
	PPST_AUDIO_RAW_PLAYER,
	PPST_AUDIO_ENCODED_PLAYER,
	PPST_AUDIO_RAW_SENDER,
	PPST_AUDIO_ENCODED_SENDER,
};

//==============================================================================
// 
//==============================================================================
enum class ProcessorState
{
	INVALID = 0,
	INITED,
	RUNNING,
	PAUSED,
	STOPED
};

//==============================================================================
// TODO: 
//==============================================================================
enum EngineMsgType
{
	ENGINE_MSG_INVALID = CLIENT_SDK_MSG_START,
	ENGINE_MSG_ADD_MEDIA_STREAM,
	ENGINE_MSG_REMOVE_MEDIA_STREAM,
	ENGINE_MSG_PLAY_PROGRESS,
	ENGINE_MSG_RUN_STATE,
	ENGINE_MSG_AUDIO_STREAM_STATS,
	ENGINE_MSG_VIDEO_STREAM_STATS,
};

}
