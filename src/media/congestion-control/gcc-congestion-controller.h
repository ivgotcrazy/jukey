#pragma once

#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "if-congestion-controller.h"
#include "thread/common-thread.h"
#include "if-timer-mgr.h"
#include "log-sink.h"

#include "call/rtp_transport_controller_send.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "api/transport/network_control.h"
#include "modules/pacing/pacing_controller.h"
#include "modules/rtp_rtcp/source/rtcp_packet/transport_feedback.h"

using namespace webrtc;

class JukeyLogSink;

namespace jukey::cc
{

//==============================================================================
// 
//==============================================================================
class GccCongestionController 
	: public ICongetionController
	, public base::ProxyUnknown
	, public base::ComObjTracer
	, public util::CommonThread
	, public webrtc::TargetTransferRateObserver
	, public webrtc::PacingController::PacketSender
{
public:
	GccCongestionController(base::IComFactory* factory, const char* owner);
	~GccCongestionController();

	// ProxyUnknown
	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// ICongetionController
	virtual void Init(IBandwidthObserver* observer, IPacketSender* sender) override;
	virtual void SetBitrateConfig(const BitrateConfig& config) override;
	virtual void OnTransportConnected() override;
	virtual void OnTransportDisconnected() override;
	virtual void OnRttUpdate(uint32_t rtt_ms) override;
	virtual void OnLossReport(const LossReport& report) override;
	virtual void OnTransportFeedback(const com::Buffer& buf) override;
	virtual void OnAddPacket(const PktSendInfo& info) override;
	virtual void OnSentPacket(const SentPktInfo& info) override;

	// TargetTransferRateObserver
	virtual void OnTargetTransferRate(TargetTransferRate rate) override;

	// PacingController::PacketSender
	virtual void SendPacket(const com::Buffer& packet,
		const PacedPacketInfo& cluster_info) override;
	virtual std::vector<com::Buffer> GeneratePadding(uint32_t size) override;

private:
	void OnTimer();
	void InitTransportSend();
	void StartTimer(uint32_t timeout_ms);
	void RestartTimer(uint32_t timeout_ms);
	void InitWebrtcLog();
	void InitTimerMgr();

private:
	base::IComFactory* m_factory = nullptr;
	IBandwidthObserver* m_observer = nullptr;
	IPacketSender* m_sender = nullptr;
	com::ITimerMgr* m_timer_mgr = nullptr;

	std::unique_ptr<TaskQueueBase, TaskQueueDeleter> m_task_queue = nullptr;
	std::unique_ptr<RtpTransportControllerSend> m_transport_send;

	com::TimerId m_timer_id = INVALID_TIMER_ID;

	std::unique_ptr<JukeyLogSink> m_log_sink;

	uint32_t kNotifyIntervalMs = 1000;
	uint64_t m_last_notify_time_us = 0;

	uint32_t m_last_bitrate_kbps = 0;

	static const uint32_t kPacerBurstIntervalMs = 5;
	static const uint32_t kDefaultMinBitrateBps = 100'000;
	static const uint32_t kDefaultMaxBitrateBps = 10'000'000;
	static const uint32_t kDefaultStartBitrateBps = 1'000'000;
	static const float kDefaultPacingFactor;
};

}
