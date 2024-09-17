/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef CALL_RTP_TRANSPORT_CONTROLLER_SEND_H_
#define CALL_RTP_TRANSPORT_CONTROLLER_SEND_H_

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "api/environment/environment.h"
#include "api/network_state_predictor.h"
#include "api/sequence_checker.h"
#include "api/task_queue/task_queue_base.h"
#include "api/task_queue/task_queue_factory.h"
#include "api/transport/network_control.h"
#include "api/units/data_rate.h"
#include "call/rtp_bitrate_configurator.h"
#include "call/rtp_transport_config.h"
#include "call/rtp_transport_controller_send_interface.h"
#include "modules/congestion_controller/rtp/control_handler.h"
#include "modules/congestion_controller/rtp/transport_feedback_adapter.h"
#include "modules/pacing/packet_router.h"
#include "modules/pacing/rtp_packet_pacer.h"
#include "modules/pacing/task_queue_paced_sender.h"
#include "modules/rtp_rtcp/include/rtp_rtcp_defines.h"
#include "rtc_base/network_route.h"
#include "rtc_base/race_checker.h"
#include "rtc_base/task_queue.h"
#include "rtc_base/rate_limiter.h"
#include "rtc_base/task_utils/repeating_task.h"

#include "simple-paced-sender.h"

namespace webrtc {
class FrameEncryptorInterface;
class RtcEventLog;

class RtpTransportControllerSend final
    : public RtpTransportControllerSendInterface,
      public NetworkLinkRtcpObserver,
      public TransportFeedbackObserver,
      public NetworkStateEstimateObserver {
 public:
  explicit RtpTransportControllerSend(
		const RtpTransportConfig& config,
		PacingController::PacketSender* packet_router);
  ~RtpTransportControllerSend() override;

  RtpTransportControllerSend(const RtpTransportControllerSend&) = delete;
  RtpTransportControllerSend& operator=(const RtpTransportControllerSend&) =
      delete;

	// 定时器驱动
	void Process();
	uint32_t NextProcessInterval();

	//////////////////////////////////////////////////////////////////////////////
  // Implements RtpTransportControllerSendInterface
  PacketRouter* packet_router() override;
  NetworkStateEstimateObserver* network_state_estimate_observer() override;
  TransportFeedbackObserver* transport_feedback_observer() override;
  RtpPacketSender* packet_sender() override;
	NetworkLinkRtcpObserver* GetRtcpObserver() override;
	int64_t GetPacerQueuingDelayMs() const override;
	absl::optional<Timestamp> GetFirstPacketTime() const override;
	StreamFeedbackProvider* GetStreamFeedbackProvider() override;

	// 平滑增益因子（需要研究下）
  void SetPacingFactor(float pacing_factor) override;

	// 如果发送队列太长有积压，说明发送不过来，可能要暂停编码
  void SetQueueTimeLimit(int limit_ms) override;

	// 注册目标码率观察者
  void RegisterTargetTransferRateObserver(
      TargetTransferRateObserver* observer) override;

	// 连接的远端端点变化
  void OnNetworkRouteChanged(absl::string_view transport_name,
                             const rtc::NetworkRoute& network_route) override;

	// 网络连接或断连
  void OnNetworkAvailability(bool network_available) override;

	// 启动周期性 ALR 带宽探测
  void EnablePeriodicAlrProbing(bool enable) override;

	// 报文发送到网络后调用
  void OnSentPacket(const rtc::SentPacket& sent_packet) override;

	// 暂未处理
  void OnReceivedPacket(const ReceivedPacket& packet_msg) override;

	// 这里可以设置 padding rate
	void SetAllocatedSendBitrateLimits(BitrateAllocationLimits limits) override;

	// 接收端设置（双方协商）
	// a=fmtp:96 x-google-max-bitrate=5000
	// a=fmtp:96 x-google-min-bitrate=1000
	// a=fmtp:96 x-google-start-bitrate=3000
  void SetSdpBitrateParameters(const BitrateConstraints& constraints) override;

	// 发送端设置
  void SetClientBitratePreferences(const BitrateSettings& preferences) override;

  void OnTransportOverheadChanged(
      size_t transport_overhead_bytes_per_packet) override;
  void AccountForAudioPacketsInPacedSender(bool account_for_audio) override;
  void IncludeOverheadInPacedSender() override;
  void EnsureStarted() override;

	//////////////////////////////////////////////////////////////////////////////
  // Implements NetworkLinkRtcpObserver interface
  void OnReceiverEstimatedMaxBitrate(Timestamp receive_time,
                                     DataRate bitrate) override;
	void OnLossReport(const TransportLossReport& report) override;
  void OnRttUpdate(Timestamp receive_time, TimeDelta rtt) override;
  void OnTransportFeedback(Timestamp receive_time,
                           const rtcp::TransportFeedback& feedback) override;

	//////////////////////////////////////////////////////////////////////////////
  // Implements TransportFeedbackObserver interface
  void OnAddPacket(const RtpPacketSendInfo& packet_info) override;

	//////////////////////////////////////////////////////////////////////////////
  // Implements NetworkStateEstimateObserver interface
  void OnRemoteNetworkEstimate(NetworkStateEstimate estimate) override;

 private:
  void MaybeCreateControllers();
  void UpdateInitialConstraints(TargetRateConstraints new_contraints);
  void UpdateControllerWithTimeInterval();
  void UpdateBitrateConstraints(const BitrateConstraints& updated);
  void UpdateStreamsConfig();
  void PostUpdates(NetworkControlUpdate update);
  void UpdateControlState();
  void UpdateCongestedState();
  absl::optional<bool> GetCongestedStateUpdate() const;
  void ProcessSentPacket(const rtc::SentPacket& sent_packet);
  void ProcessSentPacketUpdates(NetworkControlUpdate updates);

  const Environment env_;

	// 报文发送器
	PacingController::PacketSender* packet_router_ = nullptr;

  RtpBitrateConfigurator bitrate_configurator_;

	bool pacer_started_;

	// 平滑发包
	jukey::cc::SimplePacedSender pacer_;

	// 注册进来的目标码率监听者
	TargetTransferRateObserver* observer_;

	// Transport Feedback 处理器
	TransportFeedbackAdapter transport_feedback_adapter_;
     
	// 用来创建 GCC 工厂
	const std::unique_ptr<NetworkControllerFactoryInterface> controller_factory_;

	// 获取目标码率辅助对象（根据网络可用性、发送队列积压情况、目标码率变换情况）
	std::unique_ptr<CongestionControlHandler> control_handler_;

	// 拥塞控制器
	std::unique_ptr<NetworkControllerInterface> controller_;

	// 定时调用拥塞控制器的时间间隔
	TimeDelta process_interval_;

  struct LossReport {
    uint32_t extended_highest_sequence_number = 0;
    int cumulative_lost = 0;
  };
	std::map<uint32_t, LossReport> last_report_blocks_;

	Timestamp last_report_block_time_;

	NetworkControllerConfig initial_config_;
	StreamsConfig streams_config_;

  const bool reset_feedback_on_route_change_;

	// 是否将发送队列中的报文认为是 inflight 报文，如果是，则需要从 pacer 中获取排队报文大小
  const bool add_pacing_to_cwin_;

	// 记录 overhead
	size_t transport_overhead_bytes_per_packet_;

	// 记录网络可用状态
	bool network_available_;

	// 记录拥塞窗口大小（从拥塞控制器获取）
	DataSize congestion_window_size_;

	// 记录网络拥塞状态
	bool is_congested_;

  // Protected by internal locks.
  RateLimiter retransmission_rate_limiter_;

	Timestamp last_process_time_;
};

}  // namespace webrtc

#endif  // CALL_RTP_TRANSPORT_CONTROLLER_SEND_H_
