#pragma once

#include "component.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "element-base.h"
#include "if-pin.h"
#include "if-pipeline.h"
#include "common/util-stats.h"
#include "common/util-dump.h"
#include "util-streamer.h"
#include "if-session-mgr.h"
#include "thread/common-thread.h"
#include "async/session-async-proxy.h"
#include "common-struct.h"
#include "if-stream-sender.h"


// yaml-cpp warning
#pragma warning( disable: 4251 )
#pragma warning( disable: 4275 )

namespace jukey::stmr
{

//==============================================================================
// 
//==============================================================================
class AudioSendElement
	: public media::util::ElementBase
	, public base::ProxyUnknown
	, public base::ComObjTracer
	, public util::CommonThread
	, public txp::IStreamSenderHandler
{
public:
	AudioSendElement(base::IComFactory* factory, const char* owner);
	~AudioSendElement();

	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IElement
	virtual com::ErrCode DoInit(com::IProperty* param) override;
	virtual com::ErrCode DoStart() override;
	virtual com::ErrCode DoStop() override;

	// ISinkPinHandler
	virtual com::ErrCode OnSinkPinData(ISinkPin* pin,
		const PinData& data) override;
	virtual jukey::com::ErrCode OnSinkPinNegotiated(ISinkPin* pin,
		const std::string& cap) override;

	// CommonThread
	virtual void OnThreadMsg(const com::CommonMsg& msg) override;

	// ISenderHandler
	virtual void OnStreamData(uint32_t channel_id,
		uint32_t user_id,
		const com::MediaStream& stream,
		const com::Buffer& buf) override;
	virtual void OnSenderFeedback(uint32_t channel_id,
		uint32_t user_id,
		const com::MediaStream& stream,
		const com::Buffer& buf) override;
	virtual void OnEncoderTargetBitrate(uint32_t channel_id,
		uint32_t user_id,
		const com::MediaStream& stream,
		uint32_t bw_kbps) override;

private:
	com::ErrCode CreateSinkPin();
	com::ErrCode ParseProperties(com::IProperty* props);

	void OnSessionData(const com::CommonMsg& msg);
	void OnSessionClosed(const com::CommonMsg& msg);
	void OnSessionCreateResult(const com::CommonMsg& msg);

	void OnVideoStreamFeedback(const com::Buffer& buf);
	void OnStartSendStreamNotify(const com::Buffer& buf);
	void OnStopSendStreamNotify(const com::Buffer& buf);

	void OnLoginSendChannelRsp(const com::Buffer& buf);
	void OnLoginSendChannelTimeout();
	void OnLoginSendChannelError(com::ErrCode ec);

	void ParseElementConfig();

	com::Buffer BuildAudioFrameData(media::AudioFrameParaSP para,
		const com::Buffer& buf);
	com::Buffer BuildStreamDataMsg(const com::Buffer& buf);

private:
	uint32_t m_sink_pin_index = 0;

	media::com::AudioCap m_sink_pin_cap;

	util::DataStatsSP m_data_stats;
	util::StatsId m_br_stats = INVALID_STATS_ID; // bitrate
	util::StatsId m_bc_stats = INVALID_STATS_ID; // bit count
	util::StatsId m_fc_stats = INVALID_STATS_ID; // frame count
	util::StatsId m_sn_stats = INVALID_STATS_ID; // sequnece number

	net::ISessionMgr* m_sess_mgr = nullptr;
	com::Address m_service_addr;
	com::MediaStream m_stream_info;

	net::SessionId m_session_id = INVALID_SESSION_ID;

	util::SessionAsyncProxySP m_async_proxy;

	uint32_t m_cur_seq = 0;

	txp::IStreamSender* m_stream_sender = nullptr;

	util::IDataDumperSP m_send_dumper;

	bool m_sending_data = false;
};

}