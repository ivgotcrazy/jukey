#pragma once

#include <functional>
#include <inttypes.h>

#include "if-media-recorder.h"
#include "if-media-dev-mgr.h"
#include "if-media-controller.h"
#include "if-session-mgr.h"


namespace jukey::sdk
{

//==============================================================================
// 
//==============================================================================
class IMediaEngineHandler
{
public:
	virtual ~IMediaEngineHandler() {}

	//
	// Notify media stream added(local and remote)
	//
	virtual void OnAddMediaStream(const com::MediaStream& stream) = 0;

	//
	// Notify media stream removed
	//
	virtual void OnRemoveMediaStream(const com::MediaStream& stream) = 0;

	//
	// Audio stream test callback
	//
	virtual void OnAudioStreamEnergy(const com::Stream& stream, 
		uint32_t energy) = 0;

	//
	// Play progress notify
	//
	virtual void OnPlayProgress(const com::MediaSrc& msrc, uint32_t prog) = 0;

	//
	// Run state
	//
	virtual void OnRunState(const std::string& desc) = 0;

	//
	// Audio stream statistics
	//
	virtual void OnAudioStreamStats(const com::MediaStream& stream, 
		const com::AudioStreamStats& stats) = 0;

	//
	// Video stream statistics
	//
	virtual void OnVideoStreamStats(const com::MediaStream& stream,
		const com::VideoStreamStats & stats) = 0;
};

//==============================================================================
// 
//==============================================================================
class IMediaEngine
{
public:
	virtual ~IMediaEngine() {}

	//
	// Initialize media engine with component path
	//
	virtual com::ErrCode Init(const std::string& com_path,
		IMediaEngineHandler* handler,
		com::MainThreadExecutor* executor) = 0;

	//
	// Initialize media engine with external component factory
	//
	virtual com::ErrCode Init(base::IComFactory* factory, 
		IMediaEngineHandler* handler,
		com::MainThreadExecutor* executor) = 0;

	//
	// Open camera
	// @param param  see @CamParam
	//
	virtual com::ErrCode OpenCamera(const com::CamParam& param) = 0;

	//
	// Close camera
	//
	virtual com::ErrCode CloseCamera(uint32_t dev_id) = 0;

	//
	// Open microphone
	// @param param  see @MicParam
	//
	virtual com::ErrCode OpenMicrophone(const com::MicParam& param) = 0;

	//
	// Close microphone
	//
	virtual com::ErrCode CloseMicrophone(uint32_t dev_id) = 0;

	//
	// Open media file
	// @file full path of media file
	//
	virtual com::ErrCode OpenMediaFile(const std::string& file) = 0;

	//
	// Close media file
	//
	virtual com::ErrCode CloseMediaFile(const std::string& file) = 0;

	//
	// Start receive stream
	//
	virtual com::ErrCode OpenNetStream(const com::MediaStream& stream,
		const std::string& addr) = 0;

	//
	// Stop receive stream
	//
	virtual com::ErrCode CloseNetStream(const com::MediaStream& stream) = 0;

	//
	// Start playing audio stream
	//
	virtual com::ErrCode StartPlayStream(const com::MediaStream& stream) = 0;

	//
	// Stop playing audio stream
	//
	virtual com::ErrCode StopPlayStream(const com::MediaStream& stream) = 0;

	//
	// Start rendering video stream
	// @param stream_id stream
	// @param wnd       render in the wnd
	//
	virtual com::ErrCode StartRenderStream(const com::MediaStream& stream,
		void* wnd) = 0;

	//
	// Stop rendering video stream
	// @param stream_id stream
	// @param wnd       which rendering
	//
	virtual com::ErrCode StopRenderStream(const com::MediaStream& stream,
		void* wnd) = 0;

	//
	// Start send stream
	//
	virtual com::ErrCode StartSendStream(const com::MediaStream& stream,
		const std::string& addr) = 0;

	//
	// Stop send stream
	//
	virtual com::ErrCode StopSendStream(const com::MediaStream& stream) = 0;

	//
	// Get stream information
	//
	virtual com::ErrCode GetStreamInfo(const std::string& stream_id,
		com::MediaStream& stream) = 0;

	//
	// One media src may has more than one stream
	//
	virtual com::ErrCode GetMediaSrcStreams(const com::MediaSrc& msrc,
		std::vector<com::MediaStream>& streams) = 0;

	//
	// Start audio stream test
	//
	virtual com::ErrCode StartAudioTest(const com::MediaStream& stream) = 0;

	//
	// Stop audio stream test
	//
	virtual com::ErrCode StopAudioTest(const com::MediaStream& stream) = 0;

	//
	// Device manager
	//
	virtual IMediaDevMgr* GetMediaDevMgr() = 0;

	//
	// Session manager
	//
	virtual net::ISessionMgr* GetSessionMgr() = 0;
	
	//
	// Create meida recorder
	//
	virtual IMediaRecorder* CreateMediaRecorder() = 0;

	//
	// Control media source
	//
	virtual IMediaControllerSP GetMediaController() = 0;
};

}

//==============================================================================
// 
//==============================================================================
extern "C" 
{
	__declspec(dllexport) jukey::sdk::IMediaEngine* __cdecl CreateMediaEngine();
	__declspec(dllexport) void __cdecl ReleaseMediaEngine();
}