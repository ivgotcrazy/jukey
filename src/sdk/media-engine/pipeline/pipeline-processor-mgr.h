#pragma once

#include <map>

#include "common-enum.h"
#include "common-error.h"
#include "common-struct.h"
#include "if-property.h"
#include "engine-common.h"
#include "if-session-mgr.h"
#include "pipeline-processor-base.h"
#include "if-bitrate-allocate-mgr.h"


namespace jukey::sdk
{

class MediaEngineImpl;

//==============================================================================
// 
//==============================================================================
class PipelineProcessorMgr : public IPipelineProcessorHandler
{
public:
	~PipelineProcessorMgr();

	com::ErrCode Init(base::IComFactory* factory, 
		net::ISessionMgr* sm, 
		com::MainThreadExecutor* executor,
		MediaEngineImpl* engine);

	com::ErrCode OpenCamera(const com::CamParam& param);
	com::ErrCode CloseCamera(uint32_t dev_id);
	com::ErrCode OpenMicrophone(const com::MicParam& param);
	com::ErrCode CloseMicrophone(uint32_t dev_id);
	com::ErrCode OpenMediaFile(const std::string& file);
	com::ErrCode CloseMediaFile(const std::string& file);
	com::ErrCode OpenNetStream(const com::MediaStream& stream,
		const std::string& addr);
	com::ErrCode CloseNetStream(const com::MediaStream& stream);
	com::ErrCode StartPlayStream(const com::MediaStream& stream);
	com::ErrCode StopPlayStream(const com::MediaStream& stream);
	com::ErrCode StartRenderStream(const com::MediaStream& stream,
		void* wnd);
	com::ErrCode StopRenderStream(const com::MediaStream& stream,
		void* wnd);
	com::ErrCode StartSendStream(const com::MediaStream& stream,
		const std::string& addr);
	com::ErrCode StopSendStream(const com::MediaStream& stream);
	com::ErrCode StartAudioTest(const com::MediaStream& stream);
	com::ErrCode StopAudioTest(const com::MediaStream& stream);

private:
	// IPipelineProcessorHandler
	virtual void OnAddStream(const com::ElementStream& stream,
		IPipelineProcessorSP processor) override;
	virtual void OnRemoveStream(const com::ElementStream& stream,
		IPipelineProcessorSP processor) override;
	virtual void OnRunState(const std::string& desc) override;
	virtual void OnPlayProgress(const com::MediaSrc& media_src,
		uint32_t progress) override;
	virtual void OnAudioStreamStats(const com::MediaStream& stream,
		const com::AudioStreamStats& stats) override;
	virtual void OnVideoStreamStats(const com::MediaStream& stream,
		const com::VideoStreamStats& stats) override;

	com::ErrCode OpenAudioStreamSrc(const com::MediaStream& stream,
		const std::string& addr);
	com::ErrCode CloseAudioStreamSrc(const com::MediaStream& stream);
	com::ErrCode OpenVideoStreamSrc(const com::MediaStream& stream,
		const std::string& addr);
	com::ErrCode CloseVideoStreamSrc(const com::MediaStream& stream);

	com::ErrCode StartPlayEncodedStream(const com::MediaStream& stream, bool sync);
	com::ErrCode StopPlayEncodedStream(const com::MediaStream& stream);
	com::ErrCode StartPlayRawStream(const com::MediaStream& stream);
	com::ErrCode StopPlayRawStream(const com::MediaStream& stream);

	com::ErrCode StartRenderEncodedStream(const com::MediaStream& stream, void* wnd);
	com::ErrCode StartRenderRawStream(const com::MediaStream& stream, void* wnd);
	com::ErrCode StopRenderEncodedStream(const com::MediaStream& stream, void* wnd);
	com::ErrCode StopRenderRawStream(const com::MediaStream& stream, void* wnd);

	com::ErrCode StartSendRawAudioStream(const com::MediaStream& stream,
		const std::string& addr);
	com::ErrCode StartSendEncodedAudioStream(const com::MediaStream& stream,
		const std::string& addr);
	com::ErrCode StartSendRawVideoStream(const com::MediaStream& stream,
		const std::string& addr);
	com::ErrCode StartSendEncodedVideoStream(const com::MediaStream& stream,
		const std::string& addr);
	com::ErrCode StopSendRawAudioStream(const com::MediaStream& stream);
	com::ErrCode StopSendEncodedAudioStream(const com::MediaStream& stream);
	com::ErrCode StopSendRawVideoStream(const com::MediaStream& stream);
	com::ErrCode StopSendEncodedVideoStream(const com::MediaStream& stream);

	stmr::ISrcPin* GetStreamSrcPin(const std::string& stream_id);
	stmr::ISrcPin* GetStreamSrcPin(const com::MediaStream& stream, 
		IPipelineProcessorSP processor,
		const com::ElementPin& pin);
	stmr::ISinkPin* GetProcessorSinkPin(const com::ElementPin& pin);

	com::ErrCode LinkProcessor(IPipelineProcessorSP src_processor, 
		const com::ElementStream& stream,
		IPipelineProcessorSP sink_processor);
	com::ErrCode UnlinkProcessor(const com::MediaStream& stream,
		IPipelineProcessorSP processor);

	typedef std::function<IPipelineProcessorSP()> ProcessorBuilder;

	com::ErrCode StartProcessorTemplate(const com::MediaStream& stream, ProcessorBuilder builder);
	com::ErrCode StopProcessor(const com::MediaStream& stream, 
		PipelineProcessorSubType pt);

private:
	friend class RawAudioPlayProcessor;
	friend class RawAudioSendProcessor;
	friend class RawVideoRenderProcessor;
	friend class RawVideoSendProcessor;
	friend class VideoStreamSrcProcessor;
	friend class AudioStreamSrcProcessor;
	friend class CameraSrcProcessor;
	friend class EncodedAudioPlayProcessor;
	friend class EncodedAudioSendProcessor;
	friend class EncodedVideoRenderProcessor;
	friend class EncodedVideoSendProcessor;
	friend class MediaFileProcessor;
	friend class MicrophoneSrcProcessor;

private:
	struct StreamEntry
	{
		com::ElementStream stream;
		IPipelineProcessorSP processor;
	};

	struct DepEntry
	{
		com::ElementPin src_pin;
		IPipelineProcessorSP src_processor;

		com::ElementPin sink_pin;
		IPipelineProcessorSP sink_processor;

		com::MediaStream stream;
	};

private:
	base::IComFactory* m_factory = nullptr;
	net::ISessionMgr* m_sess_mgr = nullptr;
	com::MainThreadExecutor* m_executor = nullptr;
	MediaEngineImpl* m_engine = nullptr;

	std::vector<IPipelineProcessorSP> m_processors;

	std::mutex m_mutex;

	// key:stream_id
	std::map<std::string, StreamEntry> m_streams;

	std::vector<DepEntry> m_deps;

	stmr::IBitrateAllocateMgr* m_br_alloc_mgr = nullptr;
};

}