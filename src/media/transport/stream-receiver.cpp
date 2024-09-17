#include "stream-receiver.h"
#include "protocol.h"
#include "log.h"
#include "util-streamer.h"
#include "common/util-time.h"
#include "transport-common.h"
#include "if-congestion-controller.h"


using namespace jukey::com;
using namespace jukey::util;

namespace jukey::txp
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
StreamReceiver::StreamReceiver(base::IComFactory* factory, const char* owner)
	: ProxyUnknown(nullptr)
	, ComObjTracer(factory, CID_STREAM_RECEIVER, owner)
	, m_factory(factory)
	, m_video_frame_buf(1024 * 1024)
	, m_fec_decoder(this)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
StreamReceiver::~StreamReceiver()
{
	if (m_timer_id != INVALID_TIMER_ID) {
		m_timer_mgr->StopTimer(m_timer_id);
		m_timer_mgr->FreeTimer(m_timer_id);
		m_timer_id = INVALID_TIMER_ID;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* StreamReceiver::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_STREAM_RECEIVER) == 0) {
		return new StreamReceiver(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* StreamReceiver::NDQueryInterface(const char* riid)
{
	if (0 == strcmp(riid, IID_STREAM_RECEIVER)) {
		return static_cast<IStreamReceiver*>(this);
	}
	else {
		return ProxyUnknown::NDQueryInterface(riid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamReceiver::InitTimer()
{
	m_timer_mgr = QUERY_TIMER_MGR(m_factory);
	assert(m_timer_mgr);

	com::TimerParam timer_param;
	timer_param.timeout = 1000;
	timer_param.timer_type = TimerType::TIMER_TYPE_LOOP;
	timer_param.timer_name = "stream receiver";
	timer_param.timer_func = [this](int64_t) { OnTimer(); };

	m_timer_id = m_timer_mgr->AllocTimer(timer_param);
	m_timer_mgr->StartTimer(m_timer_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamReceiver::InitStats()
{
	m_data_stats.reset(new util::DataStats(m_factory, g_txp_logger,
		m_stream.stream.stream_id, false));
	m_data_stats->Start();

	StatsParam i_rtx_send("rtx_send", StatsType::IACCU, 2000);
	m_i_rtx_send = m_data_stats->AddStats(i_rtx_send);

	StatsParam i_rtx_recv("rtx_recv", StatsType::IACCU, 2000);
	m_i_rtx_recv = m_data_stats->AddStats(i_rtx_recv);

	StatsParam i_rtx_br("rtx_br", StatsType::IACCU, 2000);
	m_i_rtx_br = m_data_stats->AddStats(i_rtx_br);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamReceiver::Init(IStreamReceiverHandler* handler, 
	uint32_t channel_id, uint32_t user_id, const com::MediaStream& stream)
{
	m_handler = handler;
	m_channel_id = channel_id;
	m_user_id = user_id;
	m_stream = stream;
	m_start_time_us = util::Now();

	if (stream.stream.stream_type == StreamType::AUDIO) {
		m_frame_unpacker.reset(new AudioFrameUnpacker(this));
	}
	else if (stream.stream.stream_type == StreamType::VIDEO) {
		m_frame_unpacker.reset(new VideoFrameUnpacker(this));
	}

	m_nack_requester.reset(new NackRequester(m_factory, this));

	InitTimer();
	InitStats();
	
	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// TODO: 处理序号回环
//------------------------------------------------------------------------------
StreamReceiver::LossStats StreamReceiver::CalcOriginLossStats()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	LossStats stats;
	stats.start_time = (uint32_t)(m_start_time_us / 1000);
	stats.end_time = (uint32_t)(util::Now() / 1000);

	// 更新起始统计时间
	m_start_time_us = util::Now();

	if (m_b_rcvr_pkt.empty()) {
		return stats;
	}

	uint32_t loss_count = 0;
	uint32_t prev_seq = *(m_b_rcvr_pkt.begin());

	for (auto iter = m_b_rcvr_pkt.begin(); iter != m_b_rcvr_pkt.end(); iter++) {
		if ((*iter) > prev_seq + 1) {
			uint32_t count = (*iter) - prev_seq - 1;
			loss_count += count;
			if (count > stats.max_consecutive_loss) {
				stats.max_consecutive_loss = count;
			}
		}
		prev_seq = (*iter);
	}

	// 统计总报文数量
	uint32_t total_pkt = (*m_b_rcvr_pkt.rbegin()) - (*m_b_rcvr_pkt.begin()) + 1;

	// 计算丢包率，转换成百分比
	stats.loss_rate = static_cast<uint32_t>(std::ceil(
		static_cast<double>(loss_count) * 100 / total_pkt));

	stats.recv_count = m_b_rcvr_pkt.size();
	stats.lost_count = loss_count;

	// 计算完后清空，进入下一个统计周期
	m_b_rcvr_pkt.clear();

	return stats;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
StreamReceiver::FrameStats StreamReceiver::CalcFrameStats()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	FrameStats stats;
	stats.frame_count = m_a_rcvr_frm.size();

	if (!m_a_rcvr_frm.empty()) {
		uint32_t loss_count = 0;
		uint32_t prev_frame = (*m_a_rcvr_frm.begin());

		for (auto frame : m_a_rcvr_frm) {
			if (frame > prev_frame + 1) {
				uint32_t loss = frame - (prev_frame + 1);
				LOG_WRN("Loss frame count:{}, frame:{}, prev:{}", loss, frame, prev_frame);
				loss_count += loss;
			}
			prev_frame = frame;
		}

		// 需要考虑两个统计周期之间的丢帧
		uint32_t back_frame = *(m_a_rcvr_frm.rbegin());
		m_a_rcvr_frm.clear();
		m_a_rcvr_frm.insert(back_frame);

		stats.frame_loss = loss_count;
	}

	return stats;
}

//------------------------------------------------------------------------------
// TODO: 这里没有考虑字节序
//------------------------------------------------------------------------------
void StreamReceiver::SendStateFeedback()
{
	LossStats b_stats = CalcOriginLossStats();
	FrameStats f_stats = CalcFrameStats();

	StateFB value;
	value.start_time = b_stats.start_time;
	value.end_time = b_stats.end_time;
	value.recv_count = b_stats.recv_count;
	value.lost_count = b_stats.lost_count;
	value.rtt = (uint16_t)m_rtt_value;
	value.olr = b_stats.loss_rate;
	value.clc = b_stats.max_consecutive_loss;
	value.flr = m_fec_decoder.GetFecLossRate();
	value.nlr = m_frame_unpacker->GetLossRate();
	value.flc = f_stats.frame_loss;
	value.fc  = f_stats.frame_count;

	uint32_t buf_len = sizeof(TLV) + sizeof(value);
	com::Buffer buf(buf_len, buf_len);

	TLV* tlv = (TLV*)DP(buf);
	tlv->type = FeedbackType::FBT_STATE;
	tlv->length = sizeof(value);
	memcpy(tlv->value, &value, sizeof(value));

	LOG_INF("[{}] Send state feedback, start:{}, end:{}, recv:{}, lost:{}, "
		"rtt:{}, olr:{}, clc:{}, flr:{}, nlr:{}, fc:{}, flc:{}",
		m_stream.stream.stream_id,
		value.start_time, value.end_time, value.recv_count, value.lost_count,
		value.rtt, value.olr, value.clc, value.flr, value.nlr, value.fc, value.flc);

	m_handler->OnReceiverFeedback(m_channel_id, m_user_id, m_stream, buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamReceiver::SendRttRequest()
{
	RttReq value;
	value.ts = util::Now();
	value.seq = ++m_last_send_rtt_seq;

	uint32_t buf_len = sizeof(TLV) + sizeof(RttReq);
	com::Buffer buf(buf_len, buf_len);

	TLV* tlv = (TLV*)DP(buf);
	tlv->type = FeedbackType::FBT_RTT_REQ;
	tlv->length = sizeof(value);
	memcpy(tlv->value, &value, sizeof(value));

	m_handler->OnReceiverFeedback(m_channel_id, m_user_id, m_stream, buf);

	LOG_DBG("Send rtt request, seq:{}, ts:{}, rtt:{}", value.seq, value.ts,
		m_rtt_value);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::Buffer StreamReceiver::BuildTransportFeedback()
{
	auto adapter = (cc::IWebrtcTfbAdapter*)m_factory->QueryInterface(
		CID_WEBRTC_TFB_ADAPTER, IID_WEBRTC_TFB_ADAPTER, "stream receiver");

	adapter->Init((uint16_t)m_received_pkts.begin()->first, 
		(uint32_t)(m_received_pkts.begin()->second / 1000), m_feedback_sn++);
	for (auto [seq, ts] : m_received_pkts) {
		adapter->AddReceivedPacket((uint16_t)seq, (uint32_t)(ts / 1000));
		LOG_DBG(">>> seq:{}, ts:{}", (uint16_t)seq, (uint32_t)(ts / 1000));
	}
	com::Buffer fb_buf = adapter->Serialize();

	uint32_t buf_len = sizeof(TLV) + fb_buf.data_len;
	com::Buffer buf(buf_len, buf_len);

	TLV* tlv = (TLV*)DP(buf);
	tlv->type = FeedbackType::FBT_TRANSPORT;
	tlv->length = fb_buf.data_len;

	memcpy(tlv->value, DP(fb_buf), fb_buf.data_len);

	adapter->Release();

	return buf;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamReceiver::SendTransportFeedback()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_received_pkts.empty()) return;

	com::Buffer buf = BuildTransportFeedback();

	m_received_pkts.clear(); // 清理数据，进入下一个统计周期

	m_handler->OnReceiverFeedback(m_channel_id, m_user_id, m_stream, buf);

	LOG_INF("Send transport feedback, seq:{}", m_feedback_sn - 1);
}

//------------------------------------------------------------------------------
// TODO: 这里没有考虑字节序
// TODO: 恢复后丢包率的计算不准确
//------------------------------------------------------------------------------
void StreamReceiver::OnTimer()
{
	SendRttRequest();
	SendStateFeedback();
	SendTransportFeedback();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Stream StreamReceiver::Stream()
{
	return m_stream.stream;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t StreamReceiver::ChannelId()
{
	return m_channel_id;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t StreamReceiver::UserId()
{
	return m_user_id;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void StreamReceiver::InputStreamData(const com::Buffer& buf)
{
	prot::FecHdr* fec_hdr = (prot::FecHdr*)DP(buf);
	
	// 统计重传报文
	if (fec_hdr->rtx == 1) {
		m_data_stats->OnData(m_i_rtx_recv, 1);
		m_data_stats->OnData(m_i_rtx_br, buf.data_len);
	}

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (fec_hdr->rtx == 0) {
			// 原始丢包统计
			m_b_rcvr_pkt.insert(fec_hdr->seq);
			// Transport Feedback
			m_received_pkts.insert(std::make_pair(fec_hdr->seq, util::Now()));
		}
	}

	LOG_FEC_FRAME_DBG("Input stream data", buf);

	prot::SegHdr* seg_hdr = (prot::SegHdr*)(DP(buf) + FEC_HDR_LEN);

	// Padding 报文不做后续处理
	if (seg_hdr->mt != (uint8_t)SegPktType::SPT_PADDING) {
		m_fec_decoder.WriteFecFrame(buf);
	}

	m_nack_requester->OnPreRecvPacket(fec_hdr->seq);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void StreamReceiver::OnRttResponse(const com::Buffer& buf)
{
	TLV* tlv = (TLV*)DP(buf);
	if (tlv->length != sizeof(RttRsp)) {
		LOG_ERR("Invalid tlv len:{}", tlv->length);
		return;
	}

	// 解析 RTT 响应中的时间戳
	RttRsp* value = (RttRsp*)(DP(buf) + sizeof(TLV));
	if (value->seq < m_last_recv_rtt_seq) {
		LOG_WRN("Outdated rtt seq:{}, current:{}", value->seq, m_last_recv_rtt_seq);
		return;
	}

	m_last_recv_rtt_seq = value->seq;

	uint64_t now = util::Now();
	if (now <= value->ts) {
		LOG_ERR("Invalid request ts:{}, now:{}", value->ts, now);
		return;
	}

	uint32_t diff = (uint32_t)std::ceil((now - value->ts) / 1000.0);
	uint32_t old_rtt = m_rtt_value;

	if (m_rtt_value == 0) {
		m_rtt_value = diff;
	}
	else {
		m_rtt_value = (m_rtt_value * 5 + diff * 5) / 10;
	}

	m_nack_requester->UpdateRTT(m_rtt_value);

	LOG_INF("Receive rtt response, seq:{}, diff:{}, old rtt:{}, new rtt:{}", 
		value->seq, diff, old_rtt, m_rtt_value);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void StreamReceiver::InputFeedback(const com::Buffer& buf)
{
	TLV* tlv = (TLV*)DP(buf);

	if (tlv->length + sizeof(TLV) != buf.data_len) {
		LOG_ERR("Invalid data len:{} or tlv len:{}", buf.data_len, tlv->length);
		return;
	}

	switch (tlv->type) {
	case FBT_RTT_RSP:
		OnRttResponse(buf);
		break;
	default:
		LOG_WRN("Unknown feedback type:{}", tlv->type);
	}
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void StreamReceiver::OnSegmentData(const com::Buffer& buf)
{
	// 携带 FEC 头
	m_nack_requester->OnRecvPacket(buf);

	// 剥除 FEC 头
	com::Buffer& seg_buf = const_cast<com::Buffer&>(buf);
	seg_buf.start_pos += FEC_HDR_LEN;
	seg_buf.data_len -= FEC_HDR_LEN;
	m_frame_unpacker->WriteSegmentData(seg_buf);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void StreamReceiver::OnFrameData(const com::Buffer& buf)
{
	uint32_t frame_seq = 0;

	if (m_stream.stream.stream_type == StreamType::AUDIO) {
		prot::AudioFrameHdr* hdr = (prot::AudioFrameHdr*)DP(buf);
		LOG_DBG("Received audio frame:{}", hdr->fseq);
		frame_seq = hdr->fseq;
	}
	else if (m_stream.stream.stream_type == StreamType::VIDEO) {
		prot::VideoFrameHdr* hdr = (prot::VideoFrameHdr*)DP(buf);
		LOG_DBG("Received video frame:{}, ts:{}", hdr->fseq, hdr->ts);
		frame_seq = hdr->fseq;
	}

	// 在这里统计补偿后丢帧（基于audio/video帧头的fseq）
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_a_rcvr_frm.insert(frame_seq);
	}

	// Callback to process
	m_handler->OnStreamFrame(m_channel_id, m_user_id, m_stream, buf);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void StreamReceiver::OnNackRequest(uint32_t count, const com::Buffer& buf)
{
	// 统计发送 NACK 项数量
	m_data_stats->OnData(m_i_rtx_send, count);

	m_handler->OnReceiverFeedback(m_channel_id, m_user_id, m_stream, buf);
}

}