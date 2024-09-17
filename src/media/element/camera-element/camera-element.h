#pragma once

#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "thread/common-thread.h"
#include "element-base.h"
#include "if-pin.h"
#include "if-pipeline.h"
#include "mf-async-reader.h"
#include "common-struct.h"
#include "util-streamer.h"
#include "pipeline-msg.h"
#include "stream-dumper.h"


// yaml-cpp warning
#pragma warning( disable: 4251 )
#pragma warning( disable: 4275 )

struct IMFActivate;
struct MFAsyncReader;
struct IMFMediaSource;
struct IMFSourceReader;

namespace jukey::stmr
{

//==============================================================================
// Camera device
//==============================================================================
class CameraElement 
	: public base::ProxyUnknown
	, public base::ComObjTracer
	, public media::util::ElementBase
	, public util::CommonThread
{
public:
	CameraElement(base::IComFactory* factory, const char* owner);
	~CameraElement();

	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// ElementBase
	virtual com::ErrCode DoInit(com::IProperty* props) override;
	virtual com::ErrCode DoStart() override;
	virtual com::ErrCode DoPause() override;
	virtual com::ErrCode DoResume() override;
	virtual com::ErrCode DoStop() override;
	virtual com::ErrCode PreProcPipelineMsg(
		const com::CommonMsg& msg) override;

	// ISrcPinHandler
	virtual com::ErrCode OnSrcPinNegotiated(ISrcPin* src_pin,
		const std::string& cap) override;

	// CommonThread
	virtual void ThreadProc() override;

private:
	com::ErrCode CreateSourceReader();
	com::ErrCode DestroySourceReader();
	com::ErrCode CreateMediaSource();
	com::ErrCode CreateSrcPin();
	com::ErrCode NotifyAddVideoStream();
	com::ErrCode NotifyDelVideoStream();
	com::ErrCode InitMediaFoundation();
	com::ErrCode OnPipelineNegotiateMsg(const com::CommonMsg& msg);
	com::ErrCode EnumerateVideoDevice();

	void ProcSampleSync();
	void ProcSampleAsync();
	void ProcFrameRateStats();
	bool UpdateAvaiCaps();
	bool ParseProperties(com::IProperty* props);
	void ParseElementConfig();

private:
	uint32_t m_src_pin_index = 0;

	media::com::VideoCap m_src_pin_cap;

	uint32_t m_device_id = 0;

	std::string m_stream_id;

	// Media Foundation
	IMFSourceReader* m_source_reader = nullptr;
	IMFMediaSource*  m_media_source  = nullptr;
	HANDLE           m_event         = NULL;
	MFAsyncReader*   m_async_reader  = nullptr;
	IMFActivate*     m_video_device  = nullptr;

	// TODO: should cofigurable
	bool     m_sync_mode = true;
	uint32_t m_frame_rate = 30;
	uint32_t m_frame_seq = 0;

	media::util::StreamDumperSP m_cam_dumper;

	std::mutex m_mutex;
};

}