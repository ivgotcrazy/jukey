#pragma once

#include <string>
#include "common-error.h"
#include "common-struct.h"

namespace jukey::sdk
{

//==============================================================================
// 
//==============================================================================
enum class PlayState
{
	INVALID   = 0,
	INITED    = 1,
	PLAYING   = 2,
	PAUSED    = 3,
	STOPED    = 4,
};

//==============================================================================
// 
//==============================================================================
class MediaPlayerHandler
{
public:
	virtual void OnOpenResult(bool result) = 0;

	virtual void OnPlayProgress(const com::MediaSrc& msrc, uint32_t prog) = 0;
};


//==============================================================================
// 
//==============================================================================
struct MediaPlayerParam
{
	std::string com_path;
	void* wnd = nullptr;
	MediaPlayerHandler* handler = nullptr;
	com::MainThreadExecutor* executor = nullptr;
};

//==============================================================================
// 
//==============================================================================
class IMediaPlayer
{
public:
	virtual com::ErrCode Init(const MediaPlayerParam& param) = 0;

	virtual com::ErrCode Open(const std::string& media_file) = 0;

	virtual com::ErrCode Start() = 0;

	virtual com::ErrCode Pause() = 0;

	virtual com::ErrCode Stop() = 0;

	virtual com::ErrCode SeekBegin() = 0;

	virtual com::ErrCode SeekEnd(uint32_t progress) = 0;

	virtual PlayState State() = 0;
};

//==============================================================================
// 
//==============================================================================
extern "C"
{
	__declspec(dllexport) jukey::sdk::IMediaPlayer* __cdecl CreateMediaPlayer();
	__declspec(dllexport) void __cdecl ReleaseMediaPlayer();
}

}