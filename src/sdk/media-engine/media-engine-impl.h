#pragma once

#include <regex>
#include <map>
#include <future>

#include "include/if-media-engine.h"
#include "if-dev-mgr.h"
#include "if-pipeline.h"
#include "thread/common-thread.h"
#include "pipeline/pipeline-processor-mgr.h"

namespace jukey::sdk
{

//==============================================================================
// Camera source element
//==============================================================================
class MediaEngineImpl 
	: public IMediaEngine
	, public stmr::IPlMsgHandler
	, public util::CommonThread
{
public:
	static MediaEngineImpl* Instance();
	static void Release();

	virtual ~MediaEngineImpl();

	// IMediaEngine
	virtual com::ErrCode Init(const std::string& com_path,
		IMediaEngineHandler* handler,
		com::MainThreadExecutor* executor) override;
	virtual com::ErrCode Init(base::IComFactory* factory,
		IMediaEngineHandler* handler,
		com::MainThreadExecutor* executor) override;
	virtual com::ErrCode OpenCamera(const com::CamParam& param) override;
	virtual com::ErrCode CloseCamera(uint32_t dev_id) override;
	virtual com::ErrCode OpenMicrophone(const com::MicParam& param) override;
	virtual com::ErrCode CloseMicrophone(uint32_t dev_id) override;
	virtual com::ErrCode OpenMediaFile(const std::string& file) override;
	virtual com::ErrCode CloseMediaFile(const std::string& file) override;
	virtual com::ErrCode OpenNetStream(const com::MediaStream& stream,
		const std::string& addr) override;
	virtual com::ErrCode CloseNetStream(
		const com::MediaStream& stream) override;
	virtual com::ErrCode StartPlayStream(const com::MediaStream& stream) override;
	virtual com::ErrCode StopPlayStream(const com::MediaStream& stream) override;
	virtual com::ErrCode StartRenderStream(const com::MediaStream& stream,
		void* wnd) override;
	virtual com::ErrCode StopRenderStream(const com::MediaStream& stream,
		void* wnd) override;
	virtual com::ErrCode StartSendStream(const com::MediaStream& stream,
		const std::string& addr) override;
	virtual com::ErrCode StopSendStream(const com::MediaStream& stream) override;
	virtual com::ErrCode StartAudioTest(const com::MediaStream& stream) override;
	virtual com::ErrCode StopAudioTest(const com::MediaStream& stream) override;
	virtual com::ErrCode GetStreamInfo(const std::string& stream_id,
		com::MediaStream& stream) override;
	virtual com::ErrCode GetMediaSrcStreams(const com::MediaSrc& msrc,
		std::vector<com::MediaStream>& streams) override;
	virtual IMediaDevMgr* GetMediaDevMgr() override;
	virtual IMediaRecorder* CreateMediaRecorder() override;
	virtual IMediaControllerSP GetMediaController() override;
	virtual net::ISessionMgr* GetSessionMgr() override;

	// IPlMsgHandler
	virtual void OnPipelineMsg(const com::CommonMsg& msg) override;

	// CommonThread
	virtual void OnThreadMsg(const com::CommonMsg& msg) override;

private:
	MediaEngineImpl();
	bool CheckInitParam(const std::string& com_path, 
		IMediaEngineHandler* handler,
		com::MainThreadExecutor* executor);
	bool CheckInitParam(base::IComFactory* factory, 
		IMediaEngineHandler* handler,
		com::MainThreadExecutor* executor);
	bool DoInit();

	void OnAddMediaStreamMsg(const com::CommonMsg& msg);
	void OnRemoveMediaStreamMsg(const com::CommonMsg& msg);
	void OnPlayProgressMsg(const com::CommonMsg& msg);
	void OnRunStateMsg(const com::CommonMsg& msg);
	void OnAudioStreamStatsMsg(const com::CommonMsg& msg);
	void OnVideoStreamStatsMsg(const com::CommonMsg& msg);

private:
	static MediaEngineImpl* s_engine; // singleton
	base::IComFactory* m_factory = nullptr;
	std::recursive_mutex m_mutex;
	com::MainThreadExecutor* m_executor = nullptr;
	std::string m_com_path;
	IMediaEngineHandler* m_handler = nullptr;
	std::promise<bool> m_init_promise;
	//MediaSrcHolderMgrSP m_msrc_mgr;
	//StreamProcessorMgrSP m_sp_mgr;
	IMediaControllerSP m_controller;
	net::ISessionMgr* m_sess_mgr = nullptr;
	PipelineProcessorMgr m_pp_mgr;
};

}