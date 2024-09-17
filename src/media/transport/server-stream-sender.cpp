#include "server-stream-sender.h"
#include "protocol.h"
#include "util-streamer.h"
#include "log.h"
#include "transport-common.h"
#include "common/util-time.h"


using namespace jukey::com;
using namespace jukey::util;
using namespace jukey::prot;
using namespace jukey::cc;

namespace jukey::txp
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ServerStreamSender::ServerStreamSender(base::IComFactory* factory, 
	const char* owner)
	: ProxyUnknown(nullptr)
	, ComObjTracer(factory, CID_SERVER_STREAM_SENDER, owner)
	, m_factory(factory)
	, m_pacing_sender(factory, this)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ServerStreamSender::~ServerStreamSender()
{
	if (m_congestion_controller) {
		m_congestion_controller->Release();
		m_congestion_controller = nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* ServerStreamSender::CreateInstance(base::IComFactory* factory,
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_SERVER_STREAM_SENDER) == 0) {
		return new ServerStreamSender(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* ServerStreamSender::NDQueryInterface(const char* riid)
{
	if (0 == strcmp(riid, IID_SERVER_STREAM_SENDER)) {
		return static_cast<IServerStreamSender*>(this);
	}
	else {
		return ServerStreamSender::NDQueryInterface(riid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode ServerStreamSender::Init(IStreamSenderHandler* sender_handler, 
	IFeedbackHandler* feedback_handler, uint32_t channel_id, uint32_t user_id, 
	const com::MediaStream& stream)
{
	m_sender_handler = sender_handler;
	m_feedback_handler = feedback_handler;
	m_channel_id = channel_id;
	m_user_id = user_id;
	m_stream = stream;

	m_congestion_controller = (cc::ICongetionController*)QI(
		CID_GCC_CONGESTION_CONTROLLER, IID_GCC_CONGESTION_CONTROLLER,
		"server stream sender");
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
bool ServerStreamSender::SetBitrateConfig(const BitrateConfig& config)
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
Stream ServerStreamSender::Stream()
{
	return m_stream.stream;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t ServerStreamSender::ChannelId()
{
	return m_channel_id;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t ServerStreamSender::UserId()
{
	return m_user_id;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerStreamSender::OnAddPacket(const com::Buffer& buf)
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
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerStreamSender::InputFecFrameData(const com::Buffer& buf)
{
	// 切流

	// 根据 R 值过滤 FEC 报文

	// 更新序列号
	prot::FecHdr* hdr = (prot::FecHdr*)DP(buf);
	hdr->seq = m_seq_allocator.AllocSeq();

	// GCC
	OnAddPacket(buf);

	m_pacing_sender.EnqueueData(buf, DP_LOW);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerStreamSender::OnStateFeedback(const com::Buffer& buf)
{
	TLV* tlv = (TLV*)DP(buf);

	StateFB value;
	std::memcpy(&value, tlv->value, tlv->length);

	LOG_INF("Receive state feedback, "
		"start_time:{}, end_time:{}, received:{}, lost:{}, "
		"rtt:{}, olr:{}, clc:{}, flr:{}, nlr:{}, fc:{}, flc:{}",
		value.start_time, value.end_time, value.recv_count, value.lost_count,
		value.rtt, value.olr, value.clc, value.flr, value.nlr, value.fc, value.flc);

	m_feedback_handler->OnStateFeedback(m_channel_id, m_user_id, 
		ToStateFeedback(value));

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
void ServerStreamSender::OnNackFeedback(const com::Buffer& buf)
{
	TLV* tlv = (TLV*)DP(buf);
	if (tlv->length + sizeof(TLV) != buf.data_len
		|| tlv->length < sizeof(uint32_t)
		|| tlv->length % sizeof(uint32_t) != 0) {
		LOG_ERR("Invalid data len:{} or tlv len:{}", buf.data_len, tlv->length);
		return;
	}

	LOG_DBG("Received nack feedback, length:{}", tlv->length);

	uint32_t nack_size = tlv->length / sizeof(uint32_t);
	std::vector<uint32_t> nack_list;

	for (uint32_t i = 0; i < nack_size; i++) {
		uint8_t* p = DP(buf) + sizeof(TLV) + i * sizeof(uint32_t);
		uint32_t seq = *(reinterpret_cast<uint32_t*>(p));
		LOG_INF("Add nack item, seq:{}", seq);
		nack_list.push_back(seq);
	}

	std::vector<com::Buffer> pkts;
	m_feedback_handler->OnNackRequest(m_channel_id, m_user_id, nack_list, pkts);
	
	// NACK 响应不经过 pacer，直接发送
	for (const auto& pkt : pkts) {
		prot::FecHdr* hdr = (prot::FecHdr*)DP(pkt);
		LOG_INF("Send nack response, seq:{}, k:{}, r:{}, group:{}, gseq:{}", 
			hdr->seq, FEC_K(hdr), FEC_R(hdr), hdr->group, (uint32_t)hdr->gseq);
		m_sender_handler->OnStreamData(m_channel_id, m_user_id, m_stream, pkt);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerStreamSender::OnRttRequest(const com::Buffer& buf)
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

	m_sender_handler->OnSenderFeedback(m_channel_id, m_user_id, m_stream, rsp_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerStreamSender::OnTransportFeedback(const com::Buffer& buf)
{
	assert(m_congestion_controller);

	com::Buffer& fb_buf = const_cast<com::Buffer&>(buf);
	fb_buf.data_len -= sizeof(TLV);
	fb_buf.start_pos += sizeof(TLV);

	LOG_INF("Received transport feedback, length:{}", fb_buf.data_len);

	m_congestion_controller->OnTransportFeedback(fb_buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerStreamSender::InputFeedbackData(const com::Buffer& buf)
{
	TLV* tlv = (TLV*)DP(buf);

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
void ServerStreamSender::OnSentPacket(const com::Buffer& buf)
{
	prot::FecHdr* fec_hdr = (prot::FecHdr*)DP(buf);

	LOG_FEC_FRAME_DBG("Send stream data", buf);

	cc::SentPktInfo info;
	info.seq = fec_hdr->seq;
	info.send_time_ms = util::Now() / 1000;
	info.size_bytes = buf.data_len;

	m_congestion_controller->OnSentPacket(info);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ServerStreamSender::OnPacingData(const com::Buffer& buf)
{
	// GCC
	OnSentPacket(buf);

	prot::FecHdr* hdr = (prot::FecHdr*)DP(buf);
	LOG_DBG("<<< Send pacing packet, seq:{}", hdr->seq);

	m_sender_handler->OnStreamData(m_channel_id, m_user_id, m_stream, buf);
}

//------------------------------------------------------------------------------
// IBandwidthObserver
//------------------------------------------------------------------------------
void ServerStreamSender::OnBandwidthUpdate(uint32_t bw_kbps)
{
	LOG_INF("Update bandwidth: {} kbps", bw_kbps);

	m_pacing_sender.SetPacingRate(bw_kbps);

	// TODO: 基于带宽进行分层切换和FEC过滤
}

//------------------------------------------------------------------------------
// IPacketSender
//------------------------------------------------------------------------------
void ServerStreamSender::SendPacket(const Buffer& buf,const PacingInfo& info)
{
	prot::FecHdr* hdr = (prot::FecHdr*)DP(buf);

	cc::PktSendInfo send_info;
	send_info.seq = hdr->seq;
	send_info.pmt = com::PktMediaType::PMT_PADDING;
	send_info.ts  = (uint32_t)util::Now();
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

	m_sender_handler->OnStreamData(m_channel_id, m_user_id, m_stream, buf);

	LOG_DBG("Send probing packet, seq:{}", hdr->seq);
}

//------------------------------------------------------------------------------
// IPacketSender
//------------------------------------------------------------------------------
std::vector<com::Buffer> ServerStreamSender::GeneratePadding(uint32_t data_size)
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

}