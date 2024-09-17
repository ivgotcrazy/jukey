#pragma once

#include "include/if-media-player.h"
#include "if-media-engine.h"
#include "com-factory.h"

namespace jukey::sdk
{

//==============================================================================
// 
//==============================================================================
class MediaPlayerImpl : public IMediaPlayer, public IMediaEngineHandler
{
public:
	static IMediaPlayer* Instance();
	static void Release();

	// IMediaPlayer
	virtual com::ErrCode Init(const MediaPlayerParam& param) override;
	virtual com::ErrCode Open(const std::string& media_file) override;
	virtual com::ErrCode Start() override;
	virtual com::ErrCode Pause() override;
	virtual com::ErrCode Stop() override;
	virtual com::ErrCode SeekBegin() override;
	virtual com::ErrCode SeekEnd(uint32_t progress) override;
	virtual PlayState State() override;

	// IMediaEngineHandler
	virtual void OnAddMediaStream(const com::MediaStream& stream) override;
	virtual void OnRemoveMediaStream(const com::MediaStream& stream) override;
	virtual void OnAudioStreamEnergy(const com::Stream& stream,
		uint32_t energy) override;
	virtual void OnPlayProgress(const com::MediaSrc& msrc, uint32_t prog) override;
	virtual void OnRunState(const std::string& desc) override;
	virtual void OnAudioStreamStats(const com::MediaStream& stream,
		const com::AudioStreamStats& stats) override;
	virtual void OnVideoStreamStats(const com::MediaStream& stream,
		const com::VideoStreamStats& stats) override;

private:
	MediaPlayerImpl();
	static MediaPlayerImpl* s_media_player;

	bool CheckParam(const MediaPlayerParam& param);

private:
	IMediaEngine* m_media_engine = nullptr;
	base::IComFactory* m_factory = nullptr;

	MediaPlayerParam m_param;

	PlayState m_state = PlayState::INVALID;

	std::string m_media_file;
};

}