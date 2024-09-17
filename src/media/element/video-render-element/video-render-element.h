#pragma once

#include "SDL.h"

#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "element-base.h"
#include "if-pin.h"
#include "if-pipeline.h"
#include "util-streamer.h"
#include "pipeline-msg.h"
#include "if-sync-mgr.h"
#include "common/util-stats.h"
#include "stream-dumper.h"
#include "sdl-renderer.h"


// yaml-cpp warning
#pragma warning( disable: 4251 )
#pragma warning( disable: 4275 )

namespace jukey::stmr
{

//==============================================================================
// 
//==============================================================================
class VideoRenderElement 
	: public base::ProxyUnknown
	, public base::ComObjTracer
	, public media::util::ElementBase
	, public util::CommonThread
	, public ISyncHandler
{
public:
	VideoRenderElement(base::IComFactory* factory, const char* owner);
	~VideoRenderElement();

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
	virtual com::ErrCode ProcSinkPinMsg(stmr::ISinkPin* pin,
		const stmr::PinMsg& msg);

	// ISinkPinHandler
	virtual com::ErrCode OnSinkPinData(ISinkPin* pin, 
		const PinData& data) override;
	virtual com::ErrCode OnSinkPinNegotiated(ISinkPin* pin,
		const std::string& cap) override;

	// ISyncHandler
	virtual void OnSyncUpdate(uint64_t timestamp) override;

private:
	com::ErrCode CreateSinkPin();
	com::ErrCode OnRemoveRenderer(const com::CommonMsg& msg);
	void ParseElementConfig();

private:
	uint32_t m_sink_pin_index = 0;

	// Configure
	void* m_render_wnd = nullptr; 

	// Synchronization
	uint64_t m_av_sync_id = 0;

	util::DataStatsSP m_data_stats;
	util::StatsId m_fr_stats = INVALID_STATS_ID;

	media::util::StreamDumperSP m_stream_dumper;

	std::shared_ptr<SdlRenderer> m_sdl_renderer;
};

}