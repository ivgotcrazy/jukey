#pragma once

#include <queue>

#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "element-base.h"
#include "common/util-common.h"
#include "util-streamer.h"
#include "pipeline-msg.h"
#include "thread/common-thread.h"
#include "if-pin.h"
#include "if-pipeline.h"
#include "if-sync-mgr.h"
#include "log.h"


namespace jukey::stmr
{

//==============================================================================
// 
//==============================================================================
class AudioMixElement 
	: public base::ProxyUnknown
	, public base::ComObjTracer
	, public media::util::ElementBase
	, public util::CommonThread
	, public ISyncHandler
{
public:
	AudioMixElement(base::IComFactory* factory, const char* owner);
	~AudioMixElement();

	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// ElementBase
	virtual com::ErrCode DoInit(com::IProperty* props) override;
	virtual com::ErrCode DoStart() override;
	virtual com::ErrCode PreProcPipelineMsg(
		const com::CommonMsg& msg) override;

	// ISinkPinHandler
	virtual com::ErrCode OnSinkPinData(ISinkPin* pin,
		const PinData& data) override;
	virtual com::ErrCode OnSinkPinNegotiated(ISinkPin* pin,
		const std::string& cap) override;

	// ISrcPinHandler
	virtual com::ErrCode OnSrcPinNegotiated(ISrcPin* pin,
		const std::string& cap) override;

	// CommonThread
	virtual void ThreadProc() override;

	// ISyncHandler
	virtual void OnSyncUpdate(uint64_t timestamp) override;

private:
	com::ErrCode CreateSrcPin();
	com::ErrCode OnAddSinkPin(const com::CommonMsg& msg);
	com::ErrCode OnRemoveSinkPin(const com::CommonMsg& msg);

private:
	// FIXME: more than one sink pin, and has src pins
	uint32_t m_sink_pin_index = 0;

	media::com::VideoCap m_sink_pin_cap;

	// Video data queue
	std::queue<PinDataSP> m_data_que;
	std::mutex m_mutex;
	std::condition_variable m_con_var;

	// Synchronization
	uint64_t m_av_sync_id = 0;
	uint64_t m_audio_pts = 0;
};

}