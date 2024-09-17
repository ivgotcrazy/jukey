#pragma once

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wmsdkidl.h>

#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "thread/common-thread.h"
#include "if-element.h"
#include "if-pin.h"
#include "if-pipeline.h"
#include "common-struct.h"
#include "util-streamer.h"
#include "element-base.h"
#include "common/util-stats.h"
#include "common/util-dump.h"
#include "common-config.h"


// yaml-cpp warning
#pragma warning( disable: 4251 )
#pragma warning( disable: 4275 )

namespace jukey::stmr
{

//==============================================================================
// 
//==============================================================================
class MicrophoneElement 
	: public media::util::ElementBase
	, public base::ProxyUnknown
	, public base::ComObjTracer
	, public util::CommonThread
{
public:
	MicrophoneElement(base::IComFactory* factory, const char* owner);
	~MicrophoneElement();

	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// ElementBase
	virtual com::ErrCode DoInit(com::IProperty* props) override;
	virtual com::ErrCode DoStart() override;
	virtual com::ErrCode PreProcPipelineMsg(
		const com::CommonMsg& msg) override;

	// ISrcPinHandler
	virtual com::ErrCode OnSrcPinNegotiated(ISrcPin* pin,
		const std::string& cap) override;

	// CommonThread
	virtual void ThreadProc() override;

private:
	com::ErrCode CreateSrcPin();
	com::ErrCode CreateSourceReader();
	com::ErrCode CreateMediaSource();
	com::ErrCode NotifyAudioStream();
	com::ErrCode OnPipelineNegotiateMsg(const com::CommonMsg& msg);
	com::ErrCode EnumMicrophoneDevice();

	void ProcSampleSync();
	void ProcFrameRateStats();
	bool UpdateAvaiCaps();
	bool ParseProperties(com::IProperty* props);
	void ParseElementConfig();

private:
	uint32_t m_src_pin_index = 0;

	media::com::AudioCap m_src_pin_cap;

	uint32_t m_device_id = 0;

	// Media Foundation
	IMFSourceReader* m_source_reader = nullptr;
	IMFMediaSource*  m_media_source = nullptr;
	HANDLE           m_event = NULL;
	IMFActivate*     m_audio_device = nullptr;

	// Capture parameters
	media::AudioChnls m_sample_chnl = media::AudioChnls::INVALID;
	media::AudioSRate m_sample_rate = media::AudioSRate::INVALID;
	media::AudioSBits m_sample_bits = media::AudioSBits::INVALID;

	uint32_t m_frame_seq = 0;

  util::DataStatsSP m_data_stats;
	util::StatsId m_br_stats = INVALID_STATS_ID;
	util::StatsId m_fr_stats = INVALID_STATS_ID;

	util::IDataDumperSP m_mic_dumper;

	std::string m_stream_id;
};

}