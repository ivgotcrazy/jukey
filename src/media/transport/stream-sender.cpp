#include "stream-sender.h"
#include "protocol.h"
#include "util-streamer.h"
#include "log.h"
#include "common/util-time.h"
#include "transport-common.h"


using namespace jukey::com;
using namespace jukey::util;
using namespace jukey::prot;

namespace jukey::txp
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
StreamSender::StreamSender(base::IComFactory* factory, const char* owner)
	: ProxyUnknown(nullptr)
	, ComObjTracer(factory, CID_STREAM_SENDER, owner)
	, m_factory(factory)
	, m_fec_encoder(this, &m_seq_allocator)
	, m_pacing_sender(factory, this)
{
	
}

//------------------------------------------------------------------------------
// TODO: 资源销毁
//------------------------------------------------------------------------------
StreamSender::~StreamSender()
{
	LOG_INF("{}", __FUNCTION__);

	if (m_congestion_controller) {
		m_congestion_controller->Release();
	}

	if (m_timer_id != INVALID_TIMER_ID) {
		m_timer_mgr->StopTimer(m_timer_id);
		m_timer_mgr->FreeTimer(m_timer_id);
		m_timer_id = INVALID_TIMER_ID;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* StreamSender::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_STREAM_SENDER) == 0) {
		return new StreamSender(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* StreamSender::NDQueryInterface(const char* riid)
{
	if (0 == strcmp(riid, IID_STREAM_SENDER)) {
		return static_cast<IStreamSender*>(this);
	}
	else {
		return ProxyUnknown::NDQueryInterface(riid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamSender::OnTimer()
{
	m_last_send_rtx_bps = m_send_rtx_bytes * 8;
	m_send_rtx_bytes = 0;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamSender::InitTimer()
{
	m_timer_mgr = QUERY_TIMER_MGR(m_factory);
	assert(m_timer_mgr);

	com::TimerParam timer_param;
	timer_param.timeout = 1000;
	timer_param.timer_type = TimerType::TIMER_TYPE_LOOP;
	timer_param.timer_name = "stream sender";
	timer_param.timer_func = [this](int64_t) { OnTimer(); };

	m_timer_id = m_timer_mgr->AllocTimer(timer_param);
	m_timer_mgr->StartTimer(m_timer_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamSender::Init(IStreamSenderHandler* handler, uint32_t channel_id,
	uint32_t user_id, const com::MediaStream& stream)
{
	m_handler = handler;
	m_channel_id = channel_id;
	m_user_id = user_id;
	m_stream = stream;

	m_frame_packer.reset(new FramePacker(stream.stream.stream_type, this));
	m_fec_param_controller.reset(new SimpleFecParamController(this, 0, 0));

	m_congestion_controller = (cc::ICongetionController*)QI(
		CID_GCC_CONGESTION_CONTROLLER, IID_GCC_CONGESTION_CONTROLLER, 
		"stream sender");
	if (!m_congestion_controller) {
		LOG_ERR("Create congestion controller failed!");
		return ERR_CODE_FAILED;
	}
	m_congestion_controller->Init(this, this);
	m_congestion_controller->OnTransportConnected();

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool StreamSender::SetBitrateConfig(const BitrateConfig& config)
{
	LOG_INF("Set bitrate config, min:{}, max:{}, start:{}",
		config.min_bitrate_kbps, config.max_bitrate_kbps, config.start_bitrate_kbps);

	if (config.min_bitrate_kbps == 0 || config.max_bitrate_kbps == 0 
		|| config.start_bitrate_kbps == 0) {
		LOG_ERR("Found 0 config!");
		return false;
	}

	if (config.min_bitrate_kbps > config.max_bitrate_kbps) {
		LOG_ERR("Min config is greater than Max config!");
		return false;
	}

	if (config.start_bitrate_kbps < config.min_bitrate_kbps
		|| config.start_bitrate_kbps > config.max_bitrate_kbps) {
		LOG_ERR("Invalid start bitrate!");
		return false;
	}

	cc::BitrateConfig cfg;
	cfg.min_bitrate_kbps = config.min_bitrate_kbps;
	cfg.max_bitrate_kbps = config.max_bitrate_kbps;
	cfg.start_bitrate_kbps = config.start_bitrate_kbps;

	m_congestion_controller->SetBitrateConfig(cfg);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Stream StreamSender::Stream()
{
	return m_stream.stream;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t StreamSender::ChannelId()
{
	return m_channel_id;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t StreamSender::UserId()
{
	return m_user_id;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamSender::InputFrameData(const com::Buffer& buf)
{
	m_frame_packer->WriteFrameData(buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamSender::UpdateFeedbackHistory(const StateFB& fb)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_state_feedback_history.size() >= kMaxStateFeedbackSize) {
		m_state_feedback_history.pop_front();
	}
	m_state_feedback_history.push_back(ToStateFeedback(fb));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamSender::OnStateFeedback(const com::Buffer& buf)
{
	TLV* tlv = (TLV*)DP(buf);

	StateFB value;
	std::memcpy(&value, tlv->value, tlv->length);

	UpdateFeedbackHistory(value);

	LOG_INF("Receive state feedback, "
		"start_time:{}, end_time:{}, received:{}, lost:{}, "
		"rtt:{}, olr:{}, clc:{}, flr:{}, nlr:{}, fc:{}, flc:{}",
		value.start_time, value.end_time, value.recv_count, value.lost_count,
		value.rtt, value.olr, value.clc, value.flr, value.nlr, value.fc, value.flc);

	m_fec_param_controller->OnStateFeedback(value);

	//////////////////////////////////////////////////////////////////////////////

	m_congestion_controller->OnRttUpdate(value.rtt);

	//////////////////////////////////////////////////////////////////////////////

	cc::LossReport report;
	report.start_time = value.start_time;
	report.end_time = value.end_time;
	report.recv_count = value.recv_count;
	report.loss_count = value.lost_count;
	report.recv_time = uint32_t(util::Now() / 1000);
	m_congestion_controller->OnLossReport(report);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamSender::OnNackFeedback(const com::Buffer& buf)
{
	TLV* tlv = (TLV*)DP(buf);

	if (tlv->length < sizeof(uint32_t) || tlv->length % sizeof(uint32_t) != 0) {
		LOG_ERR("Invalid tlv len:{}", tlv->length);
		return;
	}

	LOG_DBG("Receive nack feedback, length:{}", tlv->length);

	uint32_t nack_size = tlv->length / sizeof(uint32_t);
	com::Buffer nack_buf;

	for (uint32_t i = 0; i < nack_size; i++) {
		uint8_t* p = DP(buf) + sizeof(TLV) + i * sizeof(uint32_t);
		uint32_t seq = *(reinterpret_cast<uint32_t*>(p));
		if (m_nack_history.FindFecFrameData(seq, nack_buf)) {
			m_handler->OnStreamData(m_channel_id, m_user_id, m_stream, nack_buf);
			m_send_rtx_bytes += nack_buf.data_len;
			LOG_INF("Send nack resposne, seq:{}", seq);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamSender::OnRttRequest(const com::Buffer& buf)
{
	TLV* req_tlv = (TLV*)DP(buf);

	if (req_tlv->length != sizeof(RttReq)) {
		LOG_ERR("Invalid tlv len:{}", req_tlv->length);
		return;
	}

	// 解析 RTT 请求中的时间戳
	RttReq* req_value = (RttReq*)(DP(buf) + TLV_HDR_LEN);

	// 原样返回 RTT 请求数据
	RttRsp rsp_value;
	rsp_value.ts = req_value->ts;
	rsp_value.seq = req_value->seq;

	uint32_t buf_len = TLV_HDR_LEN + sizeof(rsp_value);
	com::Buffer rsp_buf(buf_len, buf_len);

	TLV* rsp_tlv = (TLV*)DP(rsp_buf);
	rsp_tlv->type = FeedbackType::FBT_RTT_RSP;
	rsp_tlv->length = sizeof(rsp_value);
	memcpy(rsp_tlv->value, &rsp_value, sizeof(rsp_value));

	m_handler->OnSenderFeedback(m_channel_id, m_user_id, m_stream, rsp_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamSender::OnTransportFeedback(const com::Buffer& buf)
{
	assert(m_congestion_controller);

	com::Buffer& b = const_cast<com::Buffer&>(buf);
	b.data_len -= sizeof(TLV);
	b.start_pos += sizeof(TLV);

	m_congestion_controller->OnTransportFeedback(buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamSender::InputFeedbackData(const com::Buffer& buf)
{
	TLV* tlv = (TLV*)DP(buf);
	if (tlv->length + sizeof(TLV) != buf.data_len) {
		LOG_ERR("Invalid data len:{} or tlv len:{}", buf.data_len, tlv->length);
		return;
	}

	switch (tlv->type) {
	case FBT_STATE:
		OnStateFeedback(buf);
		break;
	case FBT_NACK:
		OnNackFeedback(buf);
		break;
	case FBT_RTT_REQ:
		OnRttRequest(buf);
		break;
	case FBT_TRANSPORT:
		OnTransportFeedback(buf);
		break;
	default:
		LOG_WRN("Unknown feedback type:{}", tlv->type);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamSender::OnSegmentData(const com::Buffer& buf)
{
	m_fec_encoder.WriteSegmentData(buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamSender::OnFecFrameData(const com::Buffer& buf)
{
	prot::FecHdr* hdr = (prot::FecHdr*)DP(buf);

	com::PktMediaType pmt;
	if (hdr->K == 0 || hdr->gseq < hdr->K) {
		if (m_stream.stream.stream_type == com::StreamType::AUDIO) {
			pmt = com::PktMediaType::PMT_AUDIO;
		}
		else if (m_stream.stream.stream_type == com::StreamType::VIDEO) {
			pmt = com::PktMediaType::PMT_VIDEO;
		}
		else {
			LOG_ERR("Invalid stream type:{}", m_stream.stream.stream_type);
			return;
		}
	}
	else {
		pmt = com::PktMediaType::PMT_FEC;
	}

	cc::PktSendInfo info;
	info.seq = hdr->seq;
	info.pmt = pmt;
	info.ts = (uint32_t)util::Now();
	info.len = buf.data_len;
	info.pacing_info = cc::PacingInfo(); // FIXME: 需要设置发送码率

	m_congestion_controller->OnAddPacket(info);

	// 平滑发送
	m_pacing_sender.EnqueueData(buf, DP_LOW);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamSender::OnPacingData(const com::Buffer& buf)
{
	// 在这里统计补偿前丢包（基于FEC头的seq，排除冗余包和重传包）
	prot::FecHdr* fec_hdr = (prot::FecHdr*)DP(buf);
	prot::SegHdr* seg_hdr = (prot::SegHdr*)(DP(buf) + FEC_HDR_LEN);

	LOG_DBG("Send stream data, seq:{}, group:{}, gseq:{}, fseq:{}, sseq:{}",
		fec_hdr->seq, fec_hdr->group, (uint32_t)fec_hdr->gseq, seg_hdr->fseq,
		seg_hdr->sseq);

	cc::SentPktInfo info;
	info.seq = fec_hdr->seq;
	info.send_time_ms = util::Now() / 1000;
	info.size_bytes = buf.data_len;

	m_congestion_controller->OnSentPacket(info);

	// Callback to send
	m_handler->OnStreamData(m_channel_id, m_user_id, m_stream, buf);

	// Save for nack request
	// Must call after sending for it will change the rtx bit
	m_nack_history.SaveFecFrameData(buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamSender::OnFecParamUpdate(uint8_t k, uint8_t r)
{
	m_fec_encoder.SetParam(k, r);
}

//------------------------------------------------------------------------------
// 这里发送的都是带宽探测报文，带宽探测报文做特殊处理
// 1）无需分片（生成时已经分好片）
// 2）无需FEC编码（K、R参数都为0）
// 3）无需平滑发送（不排队，立即发送）
//------------------------------------------------------------------------------
void StreamSender::SendPacket(const com::Buffer& buf, const cc::PacingInfo& info)
{
	prot::FecHdr* hdr = (prot::FecHdr*)DP(buf);

	cc::PktSendInfo send_info;
	send_info.seq = hdr->seq;
	send_info.pmt = com::PktMediaType::PMT_PADDING;
	send_info.ts = (uint32_t)util::Now();
	send_info.len = buf.data_len;
	send_info.pacing_info = info;

	m_congestion_controller->OnAddPacket(send_info);

	//////////////////////////////////////////////////////////////////////////////

	cc::SentPktInfo sent_info;
	sent_info.seq = hdr->seq;
	sent_info.send_time_ms = util::Now() / 1000;
	sent_info.size_bytes = buf.data_len;

	m_congestion_controller->OnSentPacket(sent_info);

	//////////////////////////////////////////////////////////////////////////////

	m_handler->OnStreamData(m_channel_id, m_user_id, m_stream, buf);

	m_pacing_sender.OnProbeSent(buf.data_len);

	LOG_DBG("Send probing packet, seq:{}", hdr->seq);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::vector<com::Buffer> StreamSender::GeneratePadding(uint32_t data_size)
{
	std::vector<com::Buffer> padding_pkts;

	while (data_size > 0) {
		uint32_t buf_len = 0;
		if (data_size > 1024) {
			buf_len = 1024;
			data_size -= 1024;
		}
		else if (data_size < sizeof(prot::FecHdr) + sizeof(prot::SegHdr)) {
			buf_len = sizeof(prot::FecHdr) + sizeof(prot::SegHdr);
			data_size = 0;
		}
		else {
			buf_len = data_size;
			data_size = 0;
		}

		com::Buffer buf(buf_len, buf_len);

		prot::FecHdr* fec_hdr = (prot::FecHdr*)DP(buf);
		fec_hdr->seq = m_seq_allocator.AllocSeq();

		prot::SegHdr* seg_hdr = (prot::SegHdr*)(DP(buf) + FEC_HDR_LEN);
		seg_hdr->mt = (uint8_t)SegPktType::SPT_PADDING;
		seg_hdr->slen = buf.data_len - sizeof(prot::FecHdr);

		padding_pkts.push_back(buf);
	}
	
	return padding_pkts;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamSender::OnBandwidthUpdate(uint32_t bw_kbps)
{
	// 设置平滑发包码率
	m_pacing_sender.SetPacingRate(bw_kbps);

	uint32_t aver_olr = 0;
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		if (!m_state_feedback_history.empty()) {
			uint32_t total_olr = 0;
			for (const auto& fb : m_state_feedback_history) {
				total_olr += fb.olr;
			}
			aver_olr = total_olr / m_state_feedback_history.size();
		}
	}

	// 最大允许冗余度为原始丢包率的 2 倍
	aver_olr *= 2;

	// 最大冗余度不能超过 50%
	if (aver_olr > 50) aver_olr = 50;
	
	// 设置最大保护比率
	m_fec_param_controller->SetMaxProtectionRate(aver_olr);
	
	// 编码码率 = 总码率 - 重传码率 - FEC 码率
	uint32_t encoder_kbps = 
		(bw_kbps - m_last_send_rtx_bps / 1000) * (100 - aver_olr) / 100;

	// 设置编码码率
	m_handler->OnEncoderTargetBitrate(m_channel_id, m_user_id, m_stream, 
		encoder_kbps);

	LOG_INF("OnBandwidthUpdate, total:{}kbps, encoder:{}kbps, rtx:{}bps, fec:{}",
		bw_kbps, encoder_kbps, m_last_send_rtx_bps, aver_olr);
}

}