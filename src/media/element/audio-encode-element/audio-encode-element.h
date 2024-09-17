#pragma once

#include <queue>
#include <list>

#include "component.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "element-base.h"
#include "if-pin.h"
#include "if-pipeline.h"
#include "util-streamer.h"
#include "opus.h"
#include "thread/common-thread.h"
#include "common/util-dump.h"
#include "common-config.h"


// yaml-cpp warning
#pragma warning( disable: 4251 )
#pragma warning( disable: 4275 )

namespace jukey::stmr
{
//==============================================================================
// Encode audio sample
//==============================================================================
class AudioEncodeElement
	: public media::util::ElementBase
	, public base::ProxyUnknown
	, public base::ComObjTracer
	, public util::CommonThread
{
public:
	AudioEncodeElement(base::IComFactory* factory, const char* owner);
	~AudioEncodeElement();

	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// ElementBase
	virtual com::ErrCode DoInit(com::IProperty* props) override;
	virtual com::ErrCode DoStart() override;

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

private:
	com::ErrCode CreateSrcPin();
	com::ErrCode CreateSinkPin();
	com::ErrCode CreateEncoder();
	void UpdatePinCap(const std::string& src_cap, const std::string& sink_cap);
	uint32_t GetQueueDataTotalLen();
	void FillAudioData(uint8_t* data, uint32_t len, PinData& pin_data);
	void ParseElementConfig();

private:
	uint32_t m_src_pin_index = 0;
	uint32_t m_sink_pin_index = 0;

	media::com::AudioCap m_sink_pin_cap;
	media::com::AudioCap m_src_pin_cap;

	uint32_t m_input_len = 0;

	OpusEncoder* m_opus_encoder = nullptr;

	std::list<PinDataSP> m_data_que;
	std::condition_variable_any m_con_var;

	util::IDataDumperSP m_be_dumper; // before encode
	util::IDataDumperSP m_ae_dumper; // after encode

	std::mutex m_mutex;
};

}