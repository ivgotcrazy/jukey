#pragma once

#include "modules/pacing/rtp_packet_pacer.h"
#include "modules/pacing/pacing_controller.h"

using namespace webrtc;

namespace jukey::cc
{

//==============================================================================
// 
//==============================================================================
class SimplePacedSender : public RtpPacketPacer
{
public:
	SimplePacedSender(PacingController::PacketSender* sender, 
		const FieldTrialsView& field_trials);

	uint32_t TimeUntilNextProcess();
	void Process();
	void InsertPacket(size_t bytes);
	
	// RtpPacketPacer
	virtual void CreateProbeClusters(
		std::vector<ProbeClusterConfig> probe_cluster_configs) override;
	virtual void Pause() override;
	virtual void Resume() override;
	virtual void SetCongested(bool congested) override;
	virtual void SetPacingRates(DataRate pacing_rate, DataRate padding_rate) override;
	virtual TimeDelta OldestPacketWaitTime() const override;
	virtual DataSize QueueSizeData() const override;
	virtual absl::optional<Timestamp> FirstSentPacketTime() const override;
	virtual TimeDelta ExpectedQueueTime() const override;
	virtual void SetQueueTimeLimit(TimeDelta limit) override;
	virtual void SetAccountForAudioPackets(bool account_for_audio) override;
	virtual void SetIncludeOverhead() override;
	virtual void SetTransportOverhead(DataSize overhead_per_packet) override;

private:
	int64_t UpdateTimeAndGetElapsedMs(int64_t now_us);
	uint32_t PaddingBytesToAdd();
	void UpdateBudgetWithElapsedTime(int64_t delta_time_ms);
	void UpdateBudgetWithBytesSent(size_t bytes_sent);
	void OnPaddingSent(int64_t now, size_t bytes_sent);
	void OnPacketSent(size_t size);

private:
	PacingController::PacketSender* sender_ = nullptr;
	
	BitrateProber prober_;
	bool paused_ = false;
	bool congested_ = false;

	uint32_t pacing_rate_kbps_ = 0;
	uint32_t padding_rate_kbps_ = 0;

	// This is the media budget, keeping track of how many bits of media
	// we can pace out during the current interval.
	IntervalBudget media_budget_;
	// This is the padding budget, keeping track of how many bits of padding we're
	// allowed to send out during the current interval. This budget will be
	// utilized when there's no media to send.
	IntervalBudget padding_budget_;

	int64_t time_last_process_us_ = 0;
	int64_t first_sent_packet_ms_ = 0;

	int64_t min_packet_limit_ms_ = 5;

	const int64_t kPausedProcessIntervalMs = 500;
	const int64_t kMaxElapsedTimeMs = 2000;
	const int64_t kMaxIntervalTimeMs = 30;
};

}

