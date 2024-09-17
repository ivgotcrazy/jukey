#pragma once

#include <mutex>

#include "include/if-stream-sender.h"
#include "if-unknown.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "thread/common-thread.h"
#include "frame-packer.h"
#include "fec-encoder.h"
#include "nack-history.h"
#include "pacing-sender.h"
#include "fec-param-controller.h"
#include "if-congestion-controller.h"
#include "seq-allocator.h"

namespace jukey::txp
{

//==============================================================================
// TODO: thread
//==============================================================================
class StreamSender 
	: public IStreamSender
	, public base::ProxyUnknown
	, public base::ComObjTracer
	, public IFramePackHandler
	, public IFecEncodeHandler
	, public IPacingSenderHandler
	, public IFecParamHandler
	, public cc::IPacketSender
	, public cc::IBandwidthObserver
{
public:
	StreamSender(base::IComFactory* factory, const char* owner);
	~StreamSender();

	// ProxyUnknown
	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IStreamSender
	virtual com::ErrCode Init(IStreamSenderHandler* handler, uint32_t user_id,
		uint32_t channel_id, const com::MediaStream& stream) override;
	virtual bool SetBitrateConfig(const BitrateConfig& config) override;
	virtual com::Stream Stream() override;
	virtual uint32_t ChannelId() override;
	virtual uint32_t UserId() override;
	virtual void InputFrameData(const com::Buffer& buf) override;
	virtual void InputFeedbackData(const com::Buffer& buf) override;

	// IFramePackerHandler
	virtual void OnSegmentData(const com::Buffer& buf) override;

	// IFecEncoderHandler
	virtual void OnFecFrameData(const com::Buffer& buf) override;

	// IPacingSenderHandler
	virtual void OnPacingData(const com::Buffer& buf) override;

	// IFecParamHandler
	virtual void OnFecParamUpdate(uint8_t k, uint8_t r) override;

	// IPacketSender
	virtual void SendPacket(const com::Buffer& buf, 
		const cc::PacingInfo& info) override;
	virtual std::vector<com::Buffer> GeneratePadding(
		uint32_t data_size) override;

	// IBandwidthObserver
	virtual void OnBandwidthUpdate(uint32_t bw_kbps) override;

private:
	void OnStateFeedback(const com::Buffer& buf);
	void OnNackFeedback(const com::Buffer& buf);
	void OnRttRequest(const com::Buffer& buf);
	void OnTransportFeedback(const com::Buffer& buf);
	void UpdateFeedbackHistory(const StateFB& fb);
	void InitTimer();
	void OnTimer();

private:
	base::IComFactory* m_factory = nullptr;
	IStreamSenderHandler* m_handler = nullptr;
	uint32_t m_channel_id = 0;
	uint32_t m_user_id = 0;
	com::MediaStream m_stream;
	std::mutex m_mutex;
	uint32_t m_seq = 0; // TODO: wrap around
	FramePackerUP m_frame_packer;
	FecEncoder m_fec_encoder;
	NackHistory m_nack_history;
	PacingSender m_pacing_sender;
	IFecParamControllerUP m_fec_param_controller;
	cc::ICongetionController* m_congestion_controller = nullptr;
	SeqAllocator m_seq_allocator;
	std::list<StateFeedback> m_state_feedback_history;
	static const uint32_t kMaxStateFeedbackSize = 8;

	uint32_t m_send_rtx_bytes = 0;
	uint32_t m_last_send_rtx_bps = 0;

	com::ITimerMgr* m_timer_mgr = nullptr;
	com::TimerId m_timer_id = 0;
};

}
