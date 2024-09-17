#pragma once

#include <queue>

#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "if-element.h"
#include "if-pin.h"
#include "util-streamer.h"
#include "thread/common-thread.h"
#include "common-define.h"
#include "element-base.h"
#include "common/util-stats.h"
#include "stream-dumper.h"


namespace jukey::stmr
{

//==============================================================================
// 
//==============================================================================
class AudioPlayElement
	: public base::ProxyUnknown
	, public base::ComObjTracer
	, public media::util::ElementBase
{
public:
	AudioPlayElement(base::IComFactory* factory, const char* owner);
	~AudioPlayElement();

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

	// Play callback
	void OnAudioSample(uint8_t* data, int len);

private:
	com::ErrCode CreateSinkPin();
	com::ErrCode OpenAudio();

	void ParseElementConfig();
	void CloseAudio();
	void OnPackedAudioSample(uint8_t* data, int len);
	void OnPlanarStereoAudioSample(uint8_t* data, int len);
	bool ParseProperties(com::IProperty* props);

private:
	uint32_t m_sink_pin_index = 0;

	// Negotiated cap
	media::com::AudioCap m_sink_pin_cap;

	// Cache to render
	std::queue<PinDataSP> m_data_que;

	// Next fill data, only support mono and stereo
	uint8_t* m_fill_lbuf = nullptr; // left
	uint8_t* m_fill_rbuf = nullptr; // right
	uint32_t m_fill_data_len = 0;
	uint32_t m_fill_data_pos = 0;

	util::DataStatsSP m_data_stats;
	util::StatsId m_recv_br_stats = INVALID_STATS_ID;
	util::StatsId m_recv_fr_stats = INVALID_STATS_ID;
	util::StatsId m_play_br_stats = INVALID_STATS_ID;
	util::StatsId m_que_len_stats = INVALID_STATS_ID;
	util::StatsId m_drop_num_stats = INVALID_STATS_ID;
	util::StatsId m_miss_num_stats = INVALID_STATS_ID;

	media::util::StreamDumperSP m_data_dumper;
	media::util::StreamDumperSP m_play_dumper;

	std::condition_variable_any m_con_var;
	std::mutex m_mutex;

	bool m_open_flag = false;

	// 是否是同步模式
	bool m_sync_mode = true;
};

}