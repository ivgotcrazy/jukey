#include "simple-paced-sender.h"

#include "log.h"
#include "common/util-time.h"

namespace jukey::cc
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SimplePacedSender::SimplePacedSender(PacingController::PacketSender* sender,
	const FieldTrialsView& field_trials)
	: sender_(sender)
	, prober_(field_trials)
	, media_budget_(0)
	, padding_budget_(0)
{
	time_last_process_us_ = util::Now();
	UpdateBudgetWithElapsedTime(min_packet_limit_ms_);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t SimplePacedSender::TimeUntilNextProcess()
{
	uint64_t now_us = util::Now();

	// 过去了多长时间
	int64_t elapsed_time_ms = (now_us - time_last_process_us_ + 500) / 1000;

	// 当暂停时，每500毫秒唤醒一次，发送一个填充包
	if (paused_)
		return (uint32_t)std::max<int64_t>(
			kPausedProcessIntervalMs - elapsed_time_ms, 0);

	// 如果正在探测，则返回下一个探测的时间
	if (prober_.is_probing()) {
		auto next_probe_time = prober_.NextProbeTime(Timestamp::Micros(now_us));
		if (next_probe_time.IsFinite()) {
			int64_t ts_us = next_probe_time.us();
			if (ts_us >= 0) {
				uint32_t time_ms = 0;
				if (ts_us > now_us) {
					time_ms = (uint32_t)((ts_us - now_us) / 1000);
				}
				LOG_INF("Next probe time delta: {}ms", time_ms);
				return time_ms;
			}
		}
		else {
			LOG_WRN("Infinite next probe time!");
		}
	}

	// 走到这里说明没有探测，那么就返回下一个处理的时间（5ms）
	return (uint32_t)std::max<int64_t>(min_packet_limit_ms_ - elapsed_time_ms, 0);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
int64_t SimplePacedSender::UpdateTimeAndGetElapsedMs(int64_t now_us) 
{
	// 计算过去的时间
	int64_t elapsed_time_ms = (now_us - time_last_process_us_ + 500) / 1000;

	// 更新最近一次处理的时间
	time_last_process_us_ = now_us;

	// 最大值限制（2000ms）
	if (elapsed_time_ms > kMaxElapsedTimeMs) {
		LOG_WRN("elapsed time:{}ms longer than expected, limiting to {}ms",
			elapsed_time_ms, kMaxElapsedTimeMs);

		elapsed_time_ms = kMaxElapsedTimeMs;
	}

	return elapsed_time_ms;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SimplePacedSender::Process()
{
	int64_t now_us = util::Now();
	int64_t elapsed_time_ms = UpdateTimeAndGetElapsedMs(now_us);

	if (paused_) return;

	// 使用平滑发送码率更新 media_budget_.
	if (elapsed_time_ms > 0) {
		media_budget_.set_target_rate_kbps(pacing_rate_kbps_);
		UpdateBudgetWithElapsedTime(elapsed_time_ms);
	}

	// 只处理带宽探测
	if (!prober_.is_probing()) return;

	// 需要发送 padding 报文大小
	uint32_t padding_size = PaddingBytesToAdd();
	if (padding_size == 0) {
		LOG_WRN("Zero padding data!");
		return;
	}

	// 获取探测信息
	PacedPacketInfo pacing_info = prober_.CurrentCluster(
		webrtc::Timestamp::Micros(now_us)).value_or(PacedPacketInfo());

	LOG_INF("Process probing:{}, min_bytes:{}, bytes_sent:{}, padding_size:{}", 
		pacing_info.probe_cluster_id,
		pacing_info.probe_cluster_min_bytes,
		pacing_info.probe_cluster_bytes_sent,
		padding_size);

	// 生成 padding 报文
	std::vector<com::Buffer> pkts = sender_->GeneratePadding(padding_size);

	uint64_t send_before_us = util::Now();

	// 发送 padding 报文
	uint32_t bytes_sent = 0;
	for (const auto& pkt : pkts) {
		sender_->SendPacket(pkt, pacing_info);
		bytes_sent += pkt.data_len;
	}

	uint64_t send_after_us = util::Now();

	if (send_after_us > send_before_us + 1000) {
		LOG_INF("### paced sender process:{}", send_after_us - send_before_us);
	}

	if (bytes_sent != 0) {
		auto now = util::Now();
		OnPaddingSent(now, bytes_sent);
		if (pacing_info.probe_cluster_id != PacedPacketInfo::kNotAProbe) {
			prober_.ProbeSent(Timestamp::Micros(now), DataSize::Bytes(bytes_sent));
		}
		else {
			LOG_ERR("Invalid probe bytes:{}", bytes_sent);
		}
	}
}

//------------------------------------------------------------------------------
// 有可能是发送探测报文，也有可能是发送填充报文（虽然都是 padding 报文）
//------------------------------------------------------------------------------
uint32_t SimplePacedSender::PaddingBytesToAdd()
{
	// 拥塞状态下不发送 padding 报文，即使是带宽探测也是这样
	if (congested_) return 0;

	uint32_t padding_bytes = 0;

	// 先获取探测报文发送大小
	int64_t recommended_probe_size = prober_.RecommendedMinProbeSize().bytes();
	if (recommended_probe_size > 0) { // 探测报文
		padding_bytes = (uint32_t)recommended_probe_size;
	}
	else { // 填充报文
		padding_bytes = (uint32_t)padding_budget_.bytes_remaining();
		LOG_INF("Padding bytes:{}", padding_bytes);
	}

	return padding_bytes;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SimplePacedSender::OnPaddingSent(int64_t now, size_t bytes_sent) 
{
	if (bytes_sent > 0) {
		UpdateBudgetWithBytesSent(bytes_sent);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SimplePacedSender::UpdateBudgetWithElapsedTime(int64_t delta_time_ms) 
{
	delta_time_ms = std::min(kMaxIntervalTimeMs, delta_time_ms);
	media_budget_.IncreaseBudget(delta_time_ms);
	padding_budget_.IncreaseBudget(delta_time_ms);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SimplePacedSender::UpdateBudgetWithBytesSent(size_t bytes_sent)
{
	media_budget_.UseBudget(bytes_sent);
	padding_budget_.UseBudget(bytes_sent);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SimplePacedSender::CreateProbeClusters(
	std::vector<ProbeClusterConfig> probe_cluster_configs)
{
	LOG_INF("Create probe clusters:");
	for (auto& config : probe_cluster_configs) {
		LOG_INF("### id:{}, bitrate:{}kbps, duration:{}ms, count:{}", config.id, 
			config.target_data_rate.kbps(), config.target_duration.ms(), 
			config.target_probe_count);
	}

	for (const ProbeClusterConfig& probe_cluster_config : probe_cluster_configs) {
		prober_.CreateProbeCluster(probe_cluster_config);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SimplePacedSender::InsertPacket(size_t bytes) 
{
	if (pacing_rate_kbps_ <= 0) {
		LOG_ERR("SetPacingRates() must be called before InsertPacket()");
		return;
	}

	// 驱动带宽探测
	prober_.OnIncomingPacket(webrtc::DataSize::Bytes(bytes));

	// 由于没有真实发送媒体报文，模拟报文发送出去
	OnPacketSent(bytes);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SimplePacedSender::OnPacketSent(size_t size) {
	if (first_sent_packet_ms_ == -1)
		first_sent_packet_ms_ = util::Now() / 1000;

	// Update media bytes sent.
	UpdateBudgetWithBytesSent(size);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SimplePacedSender::Pause()
{
	LOG_DBG("{}", __FUNCTION__);

	if (!paused_) {
		LOG_INF("Paused");
		paused_ = true;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SimplePacedSender::Resume()
{
	LOG_DBG("{}", __FUNCTION__);

	if (paused_) {
		LOG_INF("Resumed");
		paused_ = false;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SimplePacedSender::SetCongested(bool congested)
{
	LOG_INF("Set congested:{}", congested);

	congested_ = congested;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SimplePacedSender::SetPacingRates(DataRate pacing_rate, 
	DataRate padding_rate)
{
	LOG_DBG("Set pacing rates, pacing:{}kbps, padding:{}kbps", pacing_rate.kbps(),
		padding_rate.kbps());

	pacing_rate_kbps_ = (uint32_t)pacing_rate.kbps();
	padding_rate_kbps_ = (uint32_t)padding_rate.kbps();

	padding_budget_.set_target_rate_kbps((int)padding_rate.kbps());

	// TODO: 为什么不设置 pacing budget？
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
TimeDelta SimplePacedSender::OldestPacketWaitTime() const
{
	LOG_INF("{}", __FUNCTION__);

	return TimeDelta::Zero();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
DataSize SimplePacedSender::QueueSizeData() const
{
	LOG_INF("{}", __FUNCTION__);

	return DataSize::Zero(); // 表示发送队列为空，没有排队
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
absl::optional<Timestamp> SimplePacedSender::FirstSentPacketTime() const
{
	LOG_INF("{}", __FUNCTION__);

	if (first_sent_packet_ms_ <= 0) {
		return absl::nullopt;
	}

	return webrtc::Timestamp::Millis(first_sent_packet_ms_);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
TimeDelta SimplePacedSender::ExpectedQueueTime() const
{
	LOG_DBG("{}", __FUNCTION__);

	return TimeDelta::Millis(0); // 表示发送队列没有排队
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SimplePacedSender::SetQueueTimeLimit(TimeDelta limit)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SimplePacedSender::SetAccountForAudioPackets(bool account_for_audio)
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SimplePacedSender::SetIncludeOverhead()
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SimplePacedSender::SetTransportOverhead(DataSize overhead_per_packet)
{
	LOG_INF("{}", __FUNCTION__);
}

}
