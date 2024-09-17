#include "gcc-congestion-controller.h"

#include <memory>

#include "log.h"
#include "common/util-time.h"
#include "common-enum.h"
#include "jukey-clock.h"

#include "api/environment/environment_factory.h"
#include "call/rtp_transport_config.h"
#include "call/rtp_transport_controller_send.h"
#include "api/transport/goog_cc_factory.h"

namespace
{

using namespace jukey::cc;
using namespace jukey::com;

webrtc::RtpPacketMediaType PmtConvert(PktMediaType pmt)
{
	switch (pmt) {
		case PktMediaType::PMT_AUDIO:
			return webrtc::RtpPacketMediaType::kAudio;
		case PktMediaType::PMT_VIDEO:
			return webrtc::RtpPacketMediaType::kVideo;
		case PktMediaType::PMT_RTX:
			return webrtc::RtpPacketMediaType::kRetransmission;
		case PktMediaType::PMT_FEC:
			return webrtc::RtpPacketMediaType::kForwardErrorCorrection;
		case PktMediaType::PMT_PADDING:
			return webrtc::RtpPacketMediaType::kPadding;
		default:
			LOG_ERR("Invalid pmt:{}", pmt);
			return webrtc::RtpPacketMediaType::kPadding;
	}
}

}

namespace jukey::cc
{

const float GccCongestionController::kDefaultPacingFactor = 1.0;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
GccCongestionController::GccCongestionController(base::IComFactory* factory,
	const char* owner)
	: ProxyUnknown(nullptr)
	, ComObjTracer(factory, CID_GCC_CONGESTION_CONTROLLER, owner)
	, CommonThread("GCC congestion controller", 256, true)
	, m_factory(factory)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
GccCongestionController::~GccCongestionController()
{
	LOG_INF("{}", __FUNCTION__);

	if (m_timer_mgr) {
		if (m_timer_id != INVALID_TIMER_ID) {
			m_timer_mgr->StopTimer(m_timer_id);
			m_timer_mgr->FreeTimer(m_timer_id);
			m_timer_id = INVALID_TIMER_ID;
		}
		m_timer_mgr->Stop();
		m_timer_mgr->Release();
		m_timer_mgr = nullptr;
	}

	rtc::LogMessage::RemoveLogToStream(m_log_sink.get());

	StopThread();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* GccCongestionController::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_GCC_CONGESTION_CONTROLLER) == 0) {
		return new GccCongestionController(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* GccCongestionController::NDQueryInterface(const char* riid)
{
	if (0 == strcmp(riid, IID_GCC_CONGESTION_CONTROLLER)) {
		return static_cast<ICongetionController*>(this);
	}
	else {
		return ProxyUnknown::NDQueryInterface(riid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GccCongestionController::InitTransportSend()
{
	auto env_factory = EnvironmentFactory();
	// 统一 WebRTC 与应用之间的时间戳
	env_factory.Set(absl::Nullable<Clock*>(new JukeyClock()));
	auto env = EnvironmentFactory().Create();

	webrtc::GoogCcFactoryConfig factory_config;
	// 不仅仅依赖 transport feedback，还会通过反馈计算丢包率、RTT等传入
	factory_config.feedback_only = false;

	RtpTransportConfig config = { .env = env };
	config.pacer_burst_interval = TimeDelta::Millis(kPacerBurstIntervalMs);
	config.bitrate_config.max_bitrate_bps = kDefaultMaxBitrateBps;
	config.bitrate_config.min_bitrate_bps = kDefaultMinBitrateBps;
	config.bitrate_config.start_bitrate_bps = kDefaultStartBitrateBps;
	config.network_controller_factory = new GoogCcNetworkControllerFactory(
		std::move(factory_config));

	m_transport_send = std::make_unique<RtpTransportControllerSend>(config, this);
	
	// 接收目标码率变化通知
	m_transport_send->RegisterTargetTransferRateObserver(this);
	
	// 默认开启 ALR 带宽探测
	m_transport_send->EnablePeriodicAlrProbing(true);

	m_transport_send->SetPacingFactor(kDefaultPacingFactor);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GccCongestionController::StartTimer(uint32_t timeout_ms)
{
	assert(m_timer_mgr);

	com::TimerParam timer_param;
	timer_param.timeout = timeout_ms;
	timer_param.timer_type = com::TimerType::TIMER_TYPE_ONCE;
	timer_param.timer_name = "congestion controller";
	timer_param.timer_func = [this](int64_t) { OnTimer(); };

	m_timer_id = m_timer_mgr->AllocTimer(timer_param);
	m_timer_mgr->StartTimer(m_timer_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GccCongestionController::RestartTimer(uint32_t timeout_ms)
{
	assert(m_timer_mgr);
	assert(m_timer_id != INVALID_TIMER_ID);

	m_timer_mgr->UpdateTimeout(m_timer_id, timeout_ms);
	m_timer_mgr->StartTimer(m_timer_id);

	LOG_DBG("Restart timer:{}", timeout_ms);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GccCongestionController::InitWebrtcLog()
{
	// 创建并添加自定义日志接收器
	m_log_sink = std::make_unique<JukeyLogSink>();
	rtc::LogMessage::AddLogToStream(m_log_sink.get(), rtc::LS_INFO);

	// 设置日志级别
	rtc::LogMessage::LogToDebug(rtc::LS_INFO);
}

//------------------------------------------------------------------------------
// 使用独立的 TimerMgr
//------------------------------------------------------------------------------
void GccCongestionController::InitTimerMgr()
{
	base::IUnknown* p = m_factory->CreateComponent(CID_TIMER_MGR, 
		"congestion controller");
	if (!p) {
		LOG_ERR("Create timer manager failed!");
		return;
	}

	m_timer_mgr = (com::ITimerMgr*)p->QueryInterface(IID_TIMER_MGR);
	if (!m_timer_mgr) {
		LOG_ERR("QueryInterface iid-timer-mgr failed!");
		return;
	}

	m_timer_mgr->Start();

	LOG_INF("Start timer manager");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GccCongestionController::Init(IBandwidthObserver* observer, 
	IPacketSender* sender)
{
	assert(observer);
	assert(sender);

	m_observer = observer;
	m_sender = sender;
	
	StartThread();

	InitTimerMgr();
	
	InitTransportSend();

	StartTimer(5); // TODO: 延迟 5ms 启动定时器

	InitWebrtcLog();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GccCongestionController::SetBitrateConfig(const BitrateConfig& config)
{
	this->Execute([this, config](util::CallParam) -> void {
		BitrateSettings settings;
		settings.max_bitrate_bps = config.max_bitrate_kbps * 1000;
		settings.min_bitrate_bps = config.min_bitrate_kbps * 1000;
		settings.start_bitrate_bps = config.start_bitrate_kbps * 1000;

		m_transport_send->SetClientBitratePreferences(settings);
	}, nullptr);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GccCongestionController::OnTimer()
{
	assert(m_transport_send);

	if (util::Now() > m_last_notify_time_us + 1000 * 1000) {
		m_observer->OnBandwidthUpdate(m_last_bitrate_kbps);
		m_last_notify_time_us = util::Now();
	}

	this->Execute([this](util::CallParam) -> void {
		uint64_t before = util::Now();
		m_transport_send->Process();
		uint64_t after = util::Now();
		// 打印处理时间较长的情况，理论上不会需要这么长的处理时间
		if (after > before + 2000) {
			LOG_INF("process time:{}", after - before);
		}
		RestartTimer(m_transport_send->NextProcessInterval());
	}, nullptr);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GccCongestionController::OnTransportConnected()
{
	this->Execute([this](util::CallParam)-> void {
		m_transport_send->OnNetworkAvailability(true);
	}, nullptr);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GccCongestionController::OnTransportDisconnected()
{
	this->Execute([this](util::CallParam)-> void {
		m_transport_send->OnNetworkAvailability(false);
	}, nullptr);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GccCongestionController::OnRttUpdate(uint32_t rtt_ms)
{
	this->Execute([this, rtt_ms](util::CallParam)-> void {
		m_transport_send->OnRttUpdate(webrtc::Timestamp::Micros(util::Now()), 
			webrtc::TimeDelta::Millis(rtt_ms));
	}, nullptr);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GccCongestionController::OnLossReport(const LossReport& report)
{
	this->Execute([this, report](util::CallParam)-> void {
		TransportLossReport tl_report;
		tl_report.packets_received_delta = report.recv_count;
		tl_report.packets_lost_delta = report.loss_count;
		tl_report.start_time = webrtc::Timestamp::Millis(report.start_time);
		tl_report.end_time = webrtc::Timestamp::Millis(report.end_time);
		tl_report.receive_time = webrtc::Timestamp::Millis(report.recv_time);
		m_transport_send->OnLossReport(tl_report);
	}, nullptr);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GccCongestionController::OnTransportFeedback(const com::Buffer& buf)
{
	auto feedback = rtcp::TransportFeedback::ParseFrom(DP(buf), buf.data_len);

	LOG_INF("Received transport feedback, feedback seq:{}, base seq:{}, count:{}", 
		feedback->GetFeedbackSequenceNumber(), feedback->GetBaseSequence(), 
		feedback->GetPacketStatusCount());

	this->Execute([this, fb=std::move(*feedback)](util::CallParam)-> void {
		m_transport_send->OnTransportFeedback(Timestamp::Micros(util::Now()), fb);
	}, nullptr);
}

//------------------------------------------------------------------------------
// TargetTransferRateObserver
//------------------------------------------------------------------------------
void GccCongestionController::OnTargetTransferRate(TargetTransferRate rate)
{
	assert(m_observer);

	if (rate.target_rate.IsInfinite()) {
		LOG_WRN("Infinite target rate!");
		return;
	}

	uint32_t prev_bitrate_kbps = m_last_bitrate_kbps;
	m_last_bitrate_kbps = (uint32_t)rate.target_rate.kbps();

	bool notify = false;
	if (util::Now() + kNotifyIntervalMs * 1000 >= m_last_notify_time_us) {
		notify = true;
	}
	else if (m_last_bitrate_kbps < prev_bitrate_kbps * 0.75) {
		notify = true;
	}
	else if (m_last_bitrate_kbps > prev_bitrate_kbps * 1.5) {
		notify = true;
	}

	if (notify) {
		LOG_INF("OnTargetTransferRate, "
			"at_time:{}, target_rate:{}, stable_target_rate:{}, cwnd_reduce_ratio:{}",
			rate.at_time.ms(), rate.target_rate.kbps(), rate.stable_target_rate.kbps(),
			rate.cwnd_reduce_ratio);

		m_observer->OnBandwidthUpdate(m_last_bitrate_kbps);

		m_last_notify_time_us = util::Now();
	}
}

//------------------------------------------------------------------------------
// PacingController::PacketSender
// 发送探测报文
//------------------------------------------------------------------------------
void GccCongestionController::SendPacket(const com::Buffer& packet,
	const PacedPacketInfo& cluster_info)
{
	assert(m_sender);

	cc::PacingInfo info;
	info.send_bps = (uint32_t)cluster_info.send_bitrate.bps();
	info.probe_cluster_bytes_sent = cluster_info.probe_cluster_bytes_sent;
	info.probe_cluster_id = cluster_info.probe_cluster_id;
	info.probe_cluster_min_bytes = cluster_info.probe_cluster_min_bytes;
	info.probe_cluster_min_probes = cluster_info.probe_cluster_min_probes;

	m_sender->SendPacket(packet, info);
}

//------------------------------------------------------------------------------
// PacingController::PacketSender
// 生成探测所需的 padding 报文
//------------------------------------------------------------------------------
std::vector<com::Buffer> GccCongestionController::GeneratePadding(uint32_t size)
{
	assert(m_sender);

	LOG_DBG("Generate padding, size:{}", size);

	return m_sender->GeneratePadding(size);
}

//------------------------------------------------------------------------------
// TODO：有可能是当前线程回调进来，看是否需要优化
//------------------------------------------------------------------------------
void GccCongestionController::OnAddPacket(const PktSendInfo& info)
{
	assert(m_transport_send);

	this->Execute([this, info](util::CallParam)->void {
		RtpPacketSendInfo rtp_info;
		rtp_info.transport_sequence_number = info.seq;
		rtp_info.rtp_timestamp = info.ts; // TODO: 这里并不是用 RTP 时间戳赋值
		rtp_info.packet_type = PmtConvert(info.pmt);
		rtp_info.length = info.len;

		rtp_info.pacing_info.send_bitrate =
			webrtc::DataRate::BitsPerSec(info.pacing_info.send_bps);
		rtp_info.pacing_info.probe_cluster_id = info.pacing_info.probe_cluster_id;
		rtp_info.pacing_info.probe_cluster_bytes_sent =
			info.pacing_info.probe_cluster_bytes_sent;
		rtp_info.pacing_info.probe_cluster_min_bytes =
			info.pacing_info.probe_cluster_min_bytes;
		rtp_info.pacing_info.probe_cluster_min_probes =
			info.pacing_info.probe_cluster_min_probes;

		m_transport_send->OnAddPacket(rtp_info);
	}, nullptr);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void GccCongestionController::OnSentPacket(const SentPktInfo& info)
{
	assert(m_transport_send);

	this->Execute([this, info](util::CallParam)->void {
		rtc::SentPacket pkt;
		pkt.packet_id = info.seq;
		pkt.send_time_ms = info.send_time_ms;

		pkt.info.packet_size_bytes = info.size_bytes;
		pkt.info.ip_overhead_bytes = 20; // IP头长度？
		pkt.info.included_in_allocation = true;
		pkt.info.included_in_feedback = true;
		pkt.info.packet_type = rtc::PacketType::kData;
		pkt.info.protocol = rtc::PacketInfoProtocolType::kUdp;

		m_transport_send->OnSentPacket(pkt);
	}, nullptr);
}

}