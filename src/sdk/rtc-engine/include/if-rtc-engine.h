#pragma once

#include <functional>
#include <inttypes.h>

#include "common-error.h"
#include "if-media-recorder.h"
#include "if-media-dev-mgr.h"

namespace jukey::sdk
{

//==============================================================================
// 
//==============================================================================
class IRtcEngineHandler
{
public:
	virtual ~IRtcEngineHandler() {}

	//
	// Notify login result
	//
	virtual void OnLoginResult(uint32_t user_id, com::ErrCode result) = 0;

	//
	// Notify remote user stop publishing media
	//
	virtual void OnLogoutResult(uint32_t user_id, com::ErrCode result) = 0;

	//
	// Notify remote user stop publishing media
	//
	virtual void OnJoinGroupResult(uint32_t group_id, com::ErrCode result) = 0;

	//
	// Notify remote user stop publishing media
	//
	virtual void OnLeaveGroupResult(uint32_t group_id, com::ErrCode result) = 0;

	//
	// Someone joined the group
	//
	virtual void OnJoinGroupNotify(uint32_t user_id, uint32_t group_id) = 0;

	//
	// Someone leaved the group
	//
	virtual void OnLeaveGroupNotify(uint32_t user_id, uint32_t group_id) = 0;

	//
	// Notify remote user stop publishing media
	//
	virtual void OnSubStreamResult(const com::MediaStream& stream,
		com::ErrCode result) = 0;

	//
	// Notify remote user start publishing media
	// @param operation true:publish, false:unpublish
	//
	virtual void OnPubStreamNotify(uint32_t group_id, 
		const com::MediaStream& stream,
		bool operation) = 0;

	//
	// Audio stream test callback
	//
	virtual void OnAudioDevEnergy(const std::string& dev_id,
		uint32_t energy) = 0;

	//
	// Notify of running state, for debugging or monitoring
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
		const com::VideoStreamStats& stats) = 0;
};

//==============================================================================
// 
//==============================================================================
struct RtcEngineParam
{
	uint32_t app_id = 0;
  uint32_t client_id = 0;
  std::string client_name;
	std::string address;
	std::string com_path;
	IRtcEngineHandler* handler = nullptr;
	com::MainThreadExecutor* executor = nullptr;
};

//==============================================================================
// 
//==============================================================================
class IRtcEngine
{
public:
	virtual ~IRtcEngine() {}

	//
	// Initialize RTC engine
	//
	virtual com::ErrCode Init(const RtcEngineParam& param) = 0;

	//
	// User login, only one user is allowed to login meanwhile
	//
	virtual com::ErrCode Login(uint32_t user_id) = 0;

	//
	// User logout, must leave group first if joined group
	//
	virtual com::ErrCode Logout() = 0;

	//
	// Join group(only one?)
	//
	virtual com::ErrCode JoinGroup(uint32_t group_id) = 0;

	//
	// Leave group
	//
	virtual com::ErrCode LeaveGroup() = 0;

	//
	// Open camera and publish stream
	// @param dev_id camera ID
	// @param param  see @CamParam
	//
	virtual com::ErrCode OpenCamera(const com::CamParam& param, void* wnd) = 0;

	//
	// Close camera and unpublish stream
	//
	virtual com::ErrCode CloseCamera(uint32_t dev_id) = 0;

	//
	// Open microphone and publish stream
	//
	virtual com::ErrCode OpenMic(const com::MicParam& param) = 0;

	//
	// Close microphone and unpublish stream
	//
	virtual com::ErrCode CloseMic(uint32_t dev_id) = 0;

	//
	// Open media file and publish stream
	// @file full path of media file
	//
	virtual com::ErrCode OpenMediaFile(const std::string& file, void* wnd) = 0;

	//
	// Close media file and unpublish stream
	//
	virtual com::ErrCode CloseMediaFile(const std::string& file) = 0;

	//
	// Start playing audio stream
	//
	virtual com::ErrCode StartRecvAudio(const com::MediaStream& stream) = 0;

	//
	// Stop playing audio stream
	//
	virtual com::ErrCode StopRecvAudio(const com::MediaStream& stream) = 0;

	//
	// Start rendering video stream
	// @param stream stream
	// @param wnd    where to render
	//
	virtual com::ErrCode StartRecvVideo(const com::MediaStream& stream,
		void* wnd) = 0;

	//
	// Stop rendering video stream
	// @param stream stream
	// @param wnd    which renderer
	//
	virtual com::ErrCode StopRecvVideo(const com::MediaStream& stream,
		void* wnd) = 0;

	//
	// Start audio stream test
	//
	virtual com::ErrCode StartMicTest(const std::string& dev_id) = 0;

	//
	// Stop audio stream test
	//
	virtual com::ErrCode StopMicTest(const std::string& dev_id) = 0;

	//
	// Start record
	//
	virtual com::ErrCode StartRecord(const std::string& ouput_file) = 0;

	//
	// Stop record
	//
	virtual com::ErrCode StopRecord() = 0;

	//
	// Get media device manager
	//
	virtual IMediaDevMgr* GetMediaDevMgr() = 0;
};

}

//==============================================================================
// 
//==============================================================================
extern "C"
{
	__declspec(dllexport) jukey::sdk::IRtcEngine* __cdecl CreateRtcEngine();
	__declspec(dllexport) void __cdecl ReleaseRtcEngine();
}