#pragma once

#include "if-media-controller.h"

namespace jukey::sdk
{

class MediaController : public IMediaController
{
public:
	//MediaController(MediaSrcHolderMgrSP mgr);
	MediaController();

	// IMediaController
	virtual MediaState State(const com::MediaSrc& msrc) override;
	virtual uint32_t Progress(const com::MediaSrc& msrc) override;
	virtual void Start(const com::MediaSrc& msrc) override;
	virtual void Pause(const com::MediaSrc& msrc) override;
	virtual void Resume(const com::MediaSrc& msrc) override;
	virtual void Stop(const com::MediaSrc& msrc) override;
	virtual void SeekBegin(const com::MediaSrc& msrc) override;
	virtual void SeekEnd(const com::MediaSrc& msrc, uint32_t prog) override;

private:
	//MediaSrcHolderMgrSP m_msrc_mgr;
};

}