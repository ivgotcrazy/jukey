/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "call/rtp_transport_controller_send.h"

#include <memory>
#include <utility>
#include <vector>

#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "api/task_queue/pending_task_safety_flag.h"
#include "api/task_queue/task_queue_base.h"
#include "api/transport/goog_cc_factory.h"
#include "api/transport/network_types.h"
#include "api/units/data_rate.h"
#include "api/units/time_delta.h"
#include "api/units/timestamp.h"
#include "modules/rtp_rtcp/source/rtcp_packet/transport_feedback.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/rate_limiter.h"

#include "common/util-time.h"

namespace webrtc {
namespace {
static const int64_t kRetransmitWindowSizeMs = 500;
static const size_t kMaxOverheadBytes = 500;

constexpr TimeDelta kPacerQueueUpdateInterval = TimeDelta::Millis(25);

TargetRateConstraints ConvertConstraints(int min_bitrate_bps,
                                         int max_bitrate_bps,
                                         int start_bitrate_bps,
                                         Clock* clock) {
  TargetRateConstraints msg;
  msg.at_time = Timestamp::Millis(clock->TimeInMilliseconds());
  msg.min_data_rate = min_bitrate_bps >= 0
                          ? DataRate::BitsPerSec(min_bitrate_bps)
                          : DataRate::Zero();
  msg.max_data_rate = max_bitrate_bps > 0
                          ? DataRate::BitsPerSec(max_bitrate_bps)
                          : DataRate::Infinity();
  if (start_bitrate_bps > 0)
    msg.starting_rate = DataRate::BitsPerSec(start_bitrate_bps);
  return msg;
}

TargetRateConstraints ConvertConstraints(const BitrateConstraints& contraints,
                                         Clock* clock) {
  return ConvertConstraints(contraints.min_bitrate_bps,
                            contraints.max_bitrate_bps,
                            contraints.start_bitrate_bps, clock);
}

bool IsRelayed(const rtc::NetworkRoute& route) {
  return route.local.uses_turn() || route.remote.uses_turn();
}
}  // namespace

RtpTransportControllerSend::RtpTransportControllerSend(
    const RtpTransportConfig& config, PacingController::PacketSender* packet_router)
    : env_(config.env),
      bitrate_configurator_(config.bitrate_config),
      pacer_started_(false),
      packet_router_(packet_router),
      pacer_(packet_router, env_.field_trials()),
      observer_(nullptr),
      controller_factory_(config.network_controller_factory),
      process_interval_(controller_factory_->GetProcessInterval()),
      last_report_block_time_(
          Timestamp::Millis(env_.clock().TimeInMilliseconds())),
      last_process_time_(Timestamp::Millis(env_.clock().TimeInMilliseconds())),
      reset_feedback_on_route_change_(
          !env_.field_trials().IsEnabled("WebRTC-Bwe-NoFeedbackReset")),
      add_pacing_to_cwin_(env_.field_trials().IsEnabled(
          "WebRTC-AddPacingToCongestionWindowPushback")),
      transport_overhead_bytes_per_packet_(0),
      network_available_(false),
      congestion_window_size_(DataSize::PlusInfinity()),
      is_congested_(false),
      retransmission_rate_limiter_(&env_.clock(), kRetransmitWindowSizeMs) {
  initial_config_.constraints =
      ConvertConstraints(config.bitrate_config, &env_.clock());
  initial_config_.event_log = &env_.event_log();
  initial_config_.key_value_config = &env_.field_trials();
  RTC_DCHECK(config.bitrate_config.start_bitrate_bps > 0);

  pacer_.SetPacingRates(
      DataRate::BitsPerSec(config.bitrate_config.start_bitrate_bps),
      DataRate::Zero());
  if (config.pacer_burst_interval) {
    // Default burst interval overriden by config.
    // pacer_.SetSendBurstInterval(*config.pacer_burst_interval);
  }
}

RtpTransportControllerSend::~RtpTransportControllerSend() {
}

// 通知目标码率变化
void RtpTransportControllerSend::UpdateControlState() {
  absl::optional<TargetTransferRate> update = control_handler_->GetUpdate();
  if (!update)
    return;
	// 暂未分析工作机制
  retransmission_rate_limiter_.SetMaxRate((uint32_t)update->target_rate.bps());
  // We won't create control_handler_ until we have an observers.
  RTC_DCHECK(observer_ != nullptr);
  observer_->OnTargetTransferRate(*update);
}

// 拥塞状态变化，设置到平滑发包模块
void RtpTransportControllerSend::UpdateCongestedState() {
  if (auto update = GetCongestedStateUpdate()) {
    is_congested_ = update.value();
    pacer_.SetCongested(update.value());
  }
}

absl::optional<bool> RtpTransportControllerSend::GetCongestedStateUpdate() const {
	// outstanding 大于拥塞窗口大小，表示拥塞
  bool congested = 
		transport_feedback_adapter_.GetOutstandingData() >= congestion_window_size_;

	// 拥塞状态变化
  if (congested != is_congested_)
    return congested;

	// 拥塞状态没有变化
  return absl::nullopt;
}

PacketRouter* RtpTransportControllerSend::packet_router() {
  //return packet_router_;
	return nullptr;
}

NetworkStateEstimateObserver*
RtpTransportControllerSend::network_state_estimate_observer() {
  return this;
}

TransportFeedbackObserver*
RtpTransportControllerSend::transport_feedback_observer() {
  return this;
}

RtpPacketSender* RtpTransportControllerSend::packet_sender() {
  return nullptr;
}

void RtpTransportControllerSend::SetAllocatedSendBitrateLimits(
    BitrateAllocationLimits limits) {
  streams_config_.min_total_allocated_bitrate = limits.min_allocatable_rate;
  streams_config_.max_padding_rate = limits.max_padding_rate;
  streams_config_.max_total_allocated_bitrate = limits.max_allocatable_rate;
  UpdateStreamsConfig();
}

void RtpTransportControllerSend::SetPacingFactor(float pacing_factor) {
  streams_config_.pacing_factor = pacing_factor;
  UpdateStreamsConfig();
}

void RtpTransportControllerSend::SetQueueTimeLimit(int limit_ms) {
  pacer_.SetQueueTimeLimit(TimeDelta::Millis(limit_ms));
}

void RtpTransportControllerSend::RegisterTargetTransferRateObserver(
    TargetTransferRateObserver* observer) {
  RTC_DCHECK(observer_ == nullptr);
  observer_ = observer;
  observer_->OnStartRateUpdate(*initial_config_.constraints.starting_rate);
  MaybeCreateControllers();
}

void RtpTransportControllerSend::OnNetworkRouteChanged(
    absl::string_view transport_name,
    const rtc::NetworkRoute& network_route) {
	// Do nothing
}

void RtpTransportControllerSend::OnNetworkAvailability(bool network_available) {
  RTC_LOG(LS_VERBOSE) << "SignalNetworkState "
                      << (network_available ? "Up" : "Down");
  NetworkAvailability msg;
  msg.at_time = Timestamp::Millis(env_.clock().TimeInMilliseconds());
  msg.network_available = network_available;
  network_available_ = network_available;
  if (network_available) {
    pacer_.Resume();
  } else {
    pacer_.Pause();
  }
  is_congested_ = false;
  pacer_.SetCongested(false);

  if (!controller_) {
    MaybeCreateControllers();
  }
  if (controller_) {
    control_handler_->SetNetworkAvailability(network_available);
    PostUpdates(controller_->OnNetworkAvailability(msg));
    UpdateControlState();
  }
  //for (auto& rtp_sender : video_rtp_senders_) {
  //  rtp_sender->OnNetworkAvailability(network_available);
  //}
}
NetworkLinkRtcpObserver* RtpTransportControllerSend::GetRtcpObserver() {
  return this;
}

StreamFeedbackProvider* RtpTransportControllerSend::GetStreamFeedbackProvider()
{
	return nullptr;
}

int64_t RtpTransportControllerSend::GetPacerQueuingDelayMs() const {
  return pacer_.OldestPacketWaitTime().ms();
}

absl::optional<Timestamp> RtpTransportControllerSend::GetFirstPacketTime()
    const {
  return pacer_.FirstSentPacketTime();
}

void RtpTransportControllerSend::EnablePeriodicAlrProbing(bool enable) {
  streams_config_.requests_alr_probing = enable;
  UpdateStreamsConfig();
}

void RtpTransportControllerSend::OnSentPacket(const rtc::SentPacket& sent_packet) {
	pacer_.InsertPacket(sent_packet.info.packet_size_bytes);
  ProcessSentPacket(sent_packet);
}

void RtpTransportControllerSend::ProcessSentPacket(const rtc::SentPacket& sent_packet) {
	// 之前通过 OnAddPacket 添加了报文，此时报文已经发送出去
  absl::optional<SentPacket> packet_msg =
      transport_feedback_adapter_.ProcessSentPacket(sent_packet);
  if (!packet_msg)
    return;

	// 检查拥塞状态是否变化
  auto congestion_update = GetCongestedStateUpdate();

	// 检查 GCC 状态是否变化
  NetworkControlUpdate control_update;
  if (controller_)
    control_update = controller_->OnSentPacket(*packet_msg);

	// 都没变化直接返回
  if (!congestion_update && !control_update.has_updates())
    return;

	// 更新拥塞状态和处理 GCC 状态变化
  ProcessSentPacketUpdates(std::move(control_update));
}

// RTC_RUN_ON(task_queue_)
void RtpTransportControllerSend::ProcessSentPacketUpdates(
    NetworkControlUpdate updates) {
  // Only update outstanding data if:
  // 1. Packet feedback is used.
  // 2. The packet has not yet received an acknowledgement.
  // 3. It is not a retransmission of an earlier packet.
  UpdateCongestedState();
  if (controller_) {
    PostUpdates(std::move(updates));
  }
}

void RtpTransportControllerSend::OnReceivedPacket(
    const ReceivedPacket& packet_msg) {
  if (controller_)
    PostUpdates(controller_->OnReceivedPacket(packet_msg));
}

void RtpTransportControllerSend::UpdateBitrateConstraints(
    const BitrateConstraints& updated) {
  TargetRateConstraints msg = ConvertConstraints(updated, &env_.clock());
  if (controller_) {
    PostUpdates(controller_->OnTargetRateConstraints(msg));
  } else {
    UpdateInitialConstraints(msg);
  }
}

void RtpTransportControllerSend::SetSdpBitrateParameters(
    const BitrateConstraints& constraints) {
  absl::optional<BitrateConstraints> updated =
      bitrate_configurator_.UpdateWithSdpParameters(constraints);
  if (updated.has_value()) {
    UpdateBitrateConstraints(*updated);
  } else {
    RTC_LOG(LS_VERBOSE)
        << "WebRTC.RtpTransportControllerSend.SetSdpBitrateParameters: "
           "nothing to update";
  }
}

void RtpTransportControllerSend::SetClientBitratePreferences(
    const BitrateSettings& preferences) {
  absl::optional<BitrateConstraints> updated =
      bitrate_configurator_.UpdateWithClientPreferences(preferences);
  if (updated.has_value()) {
    UpdateBitrateConstraints(*updated);
  } else {
    RTC_LOG(LS_VERBOSE)
        << "WebRTC.RtpTransportControllerSend.SetClientBitratePreferences: "
           "nothing to update";
  }
}

void RtpTransportControllerSend::OnTransportOverheadChanged(
    size_t transport_overhead_bytes_per_packet) {
  if (transport_overhead_bytes_per_packet >= kMaxOverheadBytes) {
    RTC_LOG(LS_ERROR) << "Transport overhead exceeds " << kMaxOverheadBytes;
    return;
  }

  pacer_.SetTransportOverhead(
      DataSize::Bytes(transport_overhead_bytes_per_packet));
}

void RtpTransportControllerSend::AccountForAudioPacketsInPacedSender(
    bool account_for_audio) {
  pacer_.SetAccountForAudioPackets(account_for_audio);
}

void RtpTransportControllerSend::IncludeOverheadInPacedSender() {
  pacer_.SetIncludeOverhead();
}

void RtpTransportControllerSend::EnsureStarted() {
  if (!pacer_started_) {
    pacer_started_ = true;
    //pacer_.EnsureStarted();
  }
}

void RtpTransportControllerSend::OnReceiverEstimatedMaxBitrate(
    Timestamp receive_time,
    DataRate bitrate) {
  RemoteBitrateReport msg;
  msg.receive_time = receive_time;
  msg.bandwidth = bitrate;
  if (controller_)
    PostUpdates(controller_->OnRemoteBitrateReport(msg));
}

void RtpTransportControllerSend::OnRttUpdate(Timestamp receive_time,
                                             TimeDelta rtt) {
  RoundTripTimeUpdate report;
  report.receive_time = receive_time;
  report.round_trip_time = rtt.RoundTo(TimeDelta::Millis(1));
  report.smoothed = false;
  if (controller_ && !report.round_trip_time.IsZero())
    PostUpdates(controller_->OnRoundTripTimeUpdate(report));
}

void RtpTransportControllerSend::OnAddPacket(
    const RtpPacketSendInfo& packet_info) {
  Timestamp creation_time =
      Timestamp::Millis(env_.clock().TimeInMilliseconds());
  transport_feedback_adapter_.AddPacket(
      packet_info, transport_overhead_bytes_per_packet_, creation_time);
}

void RtpTransportControllerSend::OnTransportFeedback(
    Timestamp receive_time,
    const rtcp::TransportFeedback& feedback) {
  absl::optional<TransportPacketsFeedback> feedback_msg =
      transport_feedback_adapter_.ProcessTransportFeedback(feedback,
                                                           receive_time);
  if (feedback_msg) {
    if (controller_)
      PostUpdates(controller_->OnTransportPacketsFeedback(*feedback_msg));

    // Only update outstanding data if any packet is first time acked.
    UpdateCongestedState();
  }
}

void RtpTransportControllerSend::OnRemoteNetworkEstimate(
    NetworkStateEstimate estimate) {
  //env_.event_log().Log(std::make_unique<RtcEventRemoteEstimate>(
  //    estimate.link_capacity_lower, estimate.link_capacity_upper));
  estimate.update_time = Timestamp::Millis(env_.clock().TimeInMilliseconds());
  if (controller_)
    PostUpdates(controller_->OnNetworkStateEstimate(estimate));
}

void RtpTransportControllerSend::MaybeCreateControllers() {
  RTC_DCHECK(!controller_);
  RTC_DCHECK(!control_handler_);

  if (!network_available_ || !observer_)
    return;
  control_handler_ = std::make_unique<CongestionControlHandler>();

  initial_config_.constraints.at_time =
      Timestamp::Millis(env_.clock().TimeInMilliseconds());
  initial_config_.stream_based_config = streams_config_;

  if (controller_factory_) {
    controller_ = controller_factory_->Create(initial_config_);
    process_interval_ = controller_factory_->GetProcessInterval();
  }

  UpdateControllerWithTimeInterval();
}

void RtpTransportControllerSend::UpdateInitialConstraints(
    TargetRateConstraints new_contraints) {
  if (!new_contraints.starting_rate)
    new_contraints.starting_rate = initial_config_.constraints.starting_rate;
  RTC_DCHECK(new_contraints.starting_rate);
  initial_config_.constraints = new_contraints;
}

void RtpTransportControllerSend::UpdateControllerWithTimeInterval() {
  RTC_DCHECK(controller_);
  ProcessInterval msg;
  msg.at_time = Timestamp::Millis(env_.clock().TimeInMilliseconds());
  if (add_pacing_to_cwin_)
    msg.pacer_queue = pacer_.QueueSizeData();
  PostUpdates(controller_->OnProcessInterval(msg));
}

void RtpTransportControllerSend::UpdateStreamsConfig() {
  streams_config_.at_time =
      Timestamp::Millis(env_.clock().TimeInMilliseconds());
  if (controller_)
    PostUpdates(controller_->OnStreamsConfig(streams_config_));
}

// POST GCC 的更新
void RtpTransportControllerSend::PostUpdates(NetworkControlUpdate update) {
	// 更新拥塞状态
  if (update.congestion_window) {
    congestion_window_size_ = *update.congestion_window;
    UpdateCongestedState();
  }

	// 更新平滑发送码率
  if (update.pacer_config) {
    pacer_.SetPacingRates(update.pacer_config->data_rate(),
                          update.pacer_config->pad_rate());
  }

	// 创建带宽探测任务
  if (!update.probe_cluster_configs.empty()) {
    pacer_.CreateProbeClusters(std::move(update.probe_cluster_configs));
  }

  if (update.target_rate) {
    control_handler_->SetTargetRate(*update.target_rate);
    UpdateControlState();
  }
}

void RtpTransportControllerSend::OnLossReport(const TransportLossReport& report) {
  if (controller_)
    PostUpdates(controller_->OnTransportLossReport(report));
  last_report_block_time_ = report.receive_time;
}

uint32_t RtpTransportControllerSend::NextProcessInterval()
{	
	return std::min((uint32_t)process_interval_.ms(), pacer_.TimeUntilNextProcess());
}

void RtpTransportControllerSend::Process()
{
	if (!control_handler_) return;

	auto now = env_.clock().CurrentTime();
	if (now.ms() + 1 >= last_process_time_.ms() + process_interval_.ms()) {
		UpdateControllerWithTimeInterval();

		control_handler_->SetPacerQueue(pacer_.ExpectedQueueTime());
		UpdateControlState();

		last_process_time_ = now;
	}

	pacer_.Process();
}

}  // namespace webrtc
