#pragma once

#include "common-struct.h"

namespace jukey::sdk
{

//==============================================================================
// 
//==============================================================================
enum class MediaState
{
	INVALID,
	INITED,
	STARTED,
	PAUSED,
	STOPED
};

//==============================================================================
// 
//==============================================================================
class IMediaController
{
public:
	virtual MediaState State(const com::MediaSrc& msrc) = 0;
	virtual uint32_t Progress(const com::MediaSrc& msrc) = 0;

	virtual void Start(const com::MediaSrc& msrc) = 0;
	virtual void Pause(const com::MediaSrc& msrc) = 0;
	virtual void Resume(const com::MediaSrc& msrc) = 0;
	virtual void Stop(const com::MediaSrc& msrc) = 0;

	virtual void SeekBegin(const com::MediaSrc& msrc) = 0;
	virtual void SeekEnd(const com::MediaSrc& msrc, uint32_t prog) = 0;
};
typedef std::shared_ptr<IMediaController> IMediaControllerSP;

}