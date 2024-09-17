#pragma once

#include "component.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "element-base.h"
#include "if-pin.h"
#include "if-pipeline.h"
#include "common/util-stats.h"
#include "util-streamer.h"
#include "if-session-mgr.h"
#include "thread/common-thread.h"
#include "async/session-async-proxy.h"
#include "if-stream-receiver.h"
#include "stream-dumper.h"
#include "if-timer-mgr.h"

// yaml-cpp warning
#pragma warning( disable: 4251 )
#pragma warning( disable: 4275 )

namespace jukey::stmr
{

//==============================================================================
// 
//==============================================================================
class VideoRecvElement
	: public media::util::ElementBase
	, public base::ProxyUnknown
	, public base::ComObjTracer
	, public util::CommonThread
	, public txp::IStreamReceiverHandler
{
public:
	VideoRecvElement(base::IComFactory* factory, const char* owner);
	~VideoRecvElement();

	// ProxyUnknown
	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IElement
	virtual com::ErrCode DoInit(com::IProperty* param) override;
	virtual com::ErrCode DoStart() override;
	virtual com::ErrCode DoStop() override;
	virtual com::ErrCode PreProcPipelineMsg(
		const com::CommonMsg& msg) override;

	// ISrcPinHandler
	virtual com::ErrCode OnSrcPinNegotiated(ISrcPin* pin,
		const std::string& cap) override;

	// CommonThread
	virtual void OnThreadMsg(const com::CommonMsg& msg) override;

	// IReceiverHandler
	virtual void OnStreamFrame(uint32_t channel_id,
		uint32_t user_id,
		const com::MediaStream& stream,
		const com::Buffer& buf) override;
	virtual void OnReceiverFeedback(uint32_t channel_id,
		uint32_t user_id,
		const com::MediaStream& stream,
		const com::Buffer& buf) override;

	// Timer callback
	void NotifyStreamStats();

private:
	com::ErrCode CreateSrcPin();
	com::ErrCode ParseProperties(com::IProperty* props);
	com::ErrCode OnPipelineNegotiateMsg(const com::CommonMsg& msg);

	void OnSessionData(const com::CommonMsg& msg);
	void OnSessionClosed(const com::CommonMsg& msg);
	void OnSessionCreateResult(const com::CommonMsg& msg);
	void OnVideoStreamData(const com::Buffer& buf);
	void OnVideoStreamFeedback(const com::Buffer& buf);
	void OnPauseRecvStreamRsp(const com::Buffer& buf);
	void OnResumeRecvStreamRsp(const com::Buffer& buf);

	void OnLoginRecvChannelRsp(const com::Buffer& buf);
	void OnLoginRecvChannelTimeout();
	void OnLoginRecvChannelError(com::ErrCode ec);

	void OnLogoutRecvChannelRsp(const com::Buffer& buf);
	void OnLogoutRecvChannelTimeout();
	void OnLogoutRecvChannelError(com::ErrCode ec);

	void TryNegotiateWithServer();

	void OnVideoNegotiateRsp(const com::Buffer& buf);
	void OnVideoNegotiateTimeout();
	void OnVideoNegotiateError(com::ErrCode ec);

	void ParseElementConfig();

private:
	uint32_t m_src_pin_index = 0;

	media::com::VideoCap m_src_pin_cap;

	net::ISessionMgr* m_sess_mgr = nullptr;
	com::Address m_service_addr;
	com::MediaStream m_stream_info;

	net::SessionId m_session_id = INVALID_SESSION_ID;

	util::DataStatsSP m_data_stats;
	util::StatsId m_fr_stats = INVALID_STATS_ID;
	util::StatsId m_br_stats = INVALID_STATS_ID;

	util::SessionAsyncProxySP m_async_proxy;

	txp::IStreamReceiver* m_stream_receiver = nullptr;

	uint32_t m_cur_seq = 0;

	// Record resolution change
	uint32_t m_last_frame_width = 0;
	uint32_t m_last_frame_height = 0;

	media::util::StreamDumperSP m_stream_dumper;

	uint32_t m_recv_chnl_id = 0;

	bool m_negotiated = false;

	bool m_receiving_data = false;

	com::ITimerMgr* m_timer_mgr = nullptr;
	com::TimerId m_timer_id = 0;
};

}