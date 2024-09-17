#pragma once

#include "component.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "element-base.h"
#include "if-pin.h"
#include "if-pipeline.h"
#include "x264.h"
#include "util-streamer.h"
#include "common/util-stats.h"
#include "stream-dumper.h"
#include "if-bitrate-allocate-mgr.h"


// yaml-cpp warning
#pragma warning( disable: 4251 )
#pragma warning( disable: 4275 )

namespace jukey::stmr
{

//==============================================================================
// 
//==============================================================================
class VideoEncodeElement
	: public media::util::ElementBase
	, public base::ProxyUnknown
	, public base::ComObjTracer
	, public stmr::IBitrateListener
{
public:
	VideoEncodeElement(base::IComFactory* factory, const char* owner);
	~VideoEncodeElement();

	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// ElementBase
	virtual com::ErrCode DoInit(com::IProperty* props) override;
	virtual com::ErrCode DoStart() override;
	virtual com::ErrCode DoPause() override;
	virtual com::ErrCode DoResume() override;
	virtual com::ErrCode DoStop() override;

	// ISinkPinHandler
	virtual com::ErrCode OnSinkPinData(ISinkPin* pin,
		const PinData& data) override;
	virtual com::ErrCode OnSinkPinNegotiated(ISinkPin* pin,
		const std::string& cap) override;

	// ISrcPinHandler
	virtual com::ErrCode OnSrcPinNegotiated(ISrcPin* pin,
		const std::string& cap) override;

	// IBitrateListener
	virtual void UpdateBitrate(uint32_t br_kbps) override;

private:
	virtual com::ErrCode ProcSinkPinMsg(stmr::ISinkPin* pin,
		const stmr::PinMsg& msg) override;

	com::ErrCode CreateSrcPin();
	com::ErrCode CreateSinkPin();
	com::ErrCode CreateEncoder();
	com::ErrCode FillX264InPicture(const PinData& data);
	void UpdatePinCap(const std::string& src_cap, const std::string& sink_cap);
	void ParseElementConfig();
	com::ErrCode ParseProperties(com::IProperty* props);
	void UpdateBitrateList(uint32_t br_kbps);
	std::optional<uint32_t> TryIncreaseBitrate();
	std::optional<uint32_t> TryDecreaseBitrate();

private:
	uint32_t m_src_pin_index = 0;
	uint32_t m_sink_pin_index = 0;
	uint32_t m_frame_seq = 0;

	media::com::VideoCap m_src_pin_cap;
	media::com::VideoCap m_sink_pin_cap;

	// x264
	x264_t* m_x264_encoder = nullptr;
	x264_picture_t m_in_pic;
	x264_picture_t m_out_pic;
	bool m_send_single_nalu = false;

	util::DataStatsSP m_data_stats;
	util::StatsId m_br_stats_id = INVALID_STATS_ID;
	util::StatsId m_fr_stats_id = INVALID_STATS_ID;

	media::util::StreamDumperSP m_stream_dumper;

	stmr::IBitrateAllocateMgr* m_br_alloc_mgr = nullptr;
	std::string m_stream_id;

	std::mutex m_mutex;

	uint32_t m_curr_bitrate_kbps = 0;
	uint64_t m_last_update_us = 0;

	std::list<uint32_t> m_bitrate_list;

	static const uint32_t kIncreaseIntervalMs = 10000;
	static const uint32_t kDecreaseIntervalMs = 2000;
};

}