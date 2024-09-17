#include "stream-server.h"
#include "log.h"
#include "protocol.h"
#include "common/util-pb.h"
#include "util-streamer.h"

using namespace jukey::com;
using namespace jukey::util;


namespace jukey::txp
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
StreamServer::StreamServer(base::IComFactory* factory, IServerHandler* handler, 
	IThread* thread, const com::MediaStream& stream)
	: m_factory(factory)
	, m_fec_encoder(this, &m_seq_allocator)
	, m_frame_packer(stream.stream.stream_type, this)
	, m_fec_param_controller(this, 0, 0)
{
	assert(factory);
	assert(handler);
	assert(thread);

	m_handler = handler;
	m_thread = thread;
	m_stream = stream;
	m_timer_mgr = QUERY_TIMER_MGR(m_factory);

	m_negotiator.reset(new ServerNegotiator(*this));

	LOG_INF("{}, stream:{}|{}", __FUNCTION__, stream.stream.stream_type,
		stream.stream.stream_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
StreamServer::~StreamServer()
{
	LOG_INF("{}", __FUNCTION__);
	
	if (m_receiver.stream_receiver) {
		m_receiver.stream_receiver->Release();
		m_receiver.stream_receiver = nullptr;
	}

	for (auto& item : m_senders) {
		item.second->stream_sender->Release();
	}
	m_senders.clear();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t StreamServer::SrcChannelId()
{
	if (m_receiver.stream_receiver) {
		return m_receiver.stream_receiver->ChannelId();
	}
	else {
		return 0;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t StreamServer::SrcUserId()
{
	if (m_receiver.stream_receiver) {
		return m_receiver.stream_receiver->UserId();
	}
	else {
		return 0;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool StreamServer::HasDstChannel(uint32_t channel_id)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (const auto& item : m_senders) {
		if (item.first == channel_id) {
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t StreamServer::DstChannelSize()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	return m_senders.size();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Stream StreamServer::Stream()
{
	return m_stream.stream;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamServer::SetStreamSender(uint32_t channel_id, uint32_t user_id)
{
	LOG_INF("Set stream sender, channel:{}, user:{}, stream:{}|{}",
		channel_id, user_id,
		m_stream.stream.stream_type,
		m_stream.stream.stream_id);

	if (m_receiver.stream_receiver) {
		LOG_ERR("Stream receiver exists");
		return ERR_CODE_FAILED;
	}

	m_receiver.stream_receiver = (IStreamReceiver*)QI(CID_STREAM_RECEIVER,
		IID_STREAM_RECEIVER, "stream server");
	if (!m_receiver.stream_receiver) {
		LOG_ERR("Create stream receiver failed!");
		return ERR_CODE_FAILED;
	}

	if (ERR_CODE_OK != m_receiver.stream_receiver->Init(this, channel_id, user_id,
		m_stream)) {
		LOG_ERR("Initialize stream receiver failed!");
		return ERR_CODE_FAILED;
	}

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamServer::RemoveStreamSender()
{
	LOG_INF("Remove stream sender, stream:{}|{}", m_stream.stream.stream_type,
		m_stream.stream.stream_id);

	if (m_receiver.stream_receiver) {
		m_receiver.stream_receiver->Release();
		m_receiver.stream_receiver = nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamServer::AddStreamReceiver(uint32_t channel_id, uint32_t user_id)
{
	LOG_INF("Add stream dst, channel:{}, user:{}, stream:{}|{}",
		channel_id, user_id,
		m_stream.stream.stream_type,
		m_stream.stream.stream_id);

	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_senders.find(channel_id) != m_senders.end()) {
		LOG_ERR("Channel exists!");
		return ERR_CODE_FAILED;
	}

	IServerStreamSender* sender = (IServerStreamSender*)QI(CID_SERVER_STREAM_SENDER,
		IID_SERVER_STREAM_SENDER, "stream server");
	if (!sender) {
		LOG_ERR("Create stream sender failed!");
		return ERR_CODE_FAILED;
	}

	ErrCode result = sender->Init(this, this, channel_id, user_id, m_stream);
	if (ERR_CODE_OK != result) {
		LOG_ERR("Initialize stream receiver failed!");
		return result;
	}

	SenderInfoSP sender_info(new SenderInfo());
	sender_info->stream_sender = sender;

	m_senders.insert(std::make_pair(channel_id, sender_info));

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode StreamServer::RemoveStreamReceiver(uint32_t channel_id)
{
	LOG_INF("Remove stream receiver, channel:{}", channel_id);

	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_senders.find(channel_id);
	if (iter == m_senders.end()) {
		LOG_ERR("Cannot find channel!");
		return ERR_CODE_FAILED;
	}

	if (iter->second->stream_sender) {
		iter->second->stream_sender->Release();
		iter->second->stream_sender = nullptr;
	}

	m_senders.erase(iter);

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// TODO: 内存优化
//------------------------------------------------------------------------------
void StreamServer::OnRecvStreamData(uint32_t channel_id, const com::Buffer& buf)
{
	if (m_receiver.stream_receiver
		&& channel_id == m_receiver.stream_receiver->ChannelId()) {
		m_receiver.stream_receiver->InputStreamData(buf);
	}
	else {
		LOG_ERR("Invalid channel:{}", channel_id);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamServer::OnRecvFeedback(uint32_t channel_id, const com::Buffer& buf)
{
	if (m_receiver.stream_receiver 
		&& m_receiver.stream_receiver->ChannelId() == channel_id) {
		m_receiver.stream_receiver->InputFeedback(buf);
	}
	else {
		auto iter = m_senders.find(channel_id);
		if (iter != m_senders.end()) {
			return iter->second->stream_sender->InputFeedbackData(buf);
		}
		else {
			LOG_ERR("Cannot find sender channel:{}", channel_id);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamServer::OnRecvChannelData(uint32_t channel_id, uint32_t mt, 
	const com::Buffer& buf)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	switch (mt) {
	case prot::MSG_STREAM_DATA:
		OnRecvStreamData(channel_id, buf);
		break;
	case prot::MSG_STREAM_FEEDBACK:
		OnRecvFeedback(channel_id, buf);
		break;
	default:
		LOG_ERR("Invalid msg type:{}", mt);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamServer::OnRecvChannelMsg(uint32_t channel_id, const com::Buffer& buf)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	prot::SigMsgHdr* sig_hdr = (prot::SigMsgHdr*)DP(buf);

	switch (sig_hdr->mt) {
	case prot::MSG_NEGOTIATE_REQ:
		m_negotiator->OnRecvNegoReq(channel_id, buf);
		break;
	case prot::MSG_NEGOTIATE_RSP:
		m_negotiator->OnRecvNegoRsp(channel_id, buf);
		break;
	default:
		LOG_ERR("Invalid msg type:{}", (uint32_t)sig_hdr->mt);
	}
}

//------------------------------------------------------------------------------
// IStreamReceiverHandler
//------------------------------------------------------------------------------
void StreamServer::OnStreamFrame(uint32_t channel_id, uint32_t user_id,
	const com::MediaStream& stream, const com::Buffer& buf)
{
	m_frame_packer.WriteFrameData(buf);
}

//------------------------------------------------------------------------------
// IStreamReceiverHandler
//------------------------------------------------------------------------------
void StreamServer::OnReceiverFeedback(uint32_t channel_id, uint32_t user_id,
	const com::MediaStream& stream, const com::Buffer& buf)
{
	m_handler->OnSendChannelData(channel_id, user_id, prot::MSG_STREAM_FEEDBACK, buf);
}

//------------------------------------------------------------------------------
// IStreamSenderHandler
//------------------------------------------------------------------------------
void StreamServer::OnStreamData(uint32_t channel_id, uint32_t user_id,
	const com::MediaStream& stream, const com::Buffer& buf)
{
	m_handler->OnSendChannelData(channel_id, user_id, prot::MSG_STREAM_DATA, buf);
}

//------------------------------------------------------------------------------
// IStreamSenderHandler
//------------------------------------------------------------------------------
void StreamServer::OnSenderFeedback(uint32_t channel_id, uint32_t user_id,
	const com::MediaStream& stream, const com::Buffer& buf)
{
	m_handler->OnSendChannelData(channel_id, user_id, prot::MSG_STREAM_FEEDBACK, 
		buf);
}

//------------------------------------------------------------------------------
// IStreamSenderHandler
//------------------------------------------------------------------------------
void StreamServer::OnEncoderTargetBitrate(uint32_t channel_id, uint32_t user_id,
	const com::MediaStream& stream, uint32_t bw_kbps)
{
	// TODO: 根据带宽调整 FEC 编码参数？？？
}

//------------------------------------------------------------------------------
// IFecEncodeHandler
//------------------------------------------------------------------------------
void StreamServer::OnFecFrameData(const com::Buffer& buf)
{
	LOG_DBG("OnFecFrameData");

	// FEC 编码出来的帧，全部扔给 Sender 取处理
	for (auto iter : m_senders) {
		iter.second->stream_sender->InputFecFrameData(buf);
	}

	// Save for nack request, it will change the rtx bit, so need clone.
	m_nack_history.SaveFecFrameData(buf.Clone()); 
}

//------------------------------------------------------------------------------
// IFramePackHandler
//------------------------------------------------------------------------------
void StreamServer::OnSegmentData(const com::Buffer& buf)
{
	LOG_DBG("OnSegmentData");

	// 打包完成后进行 FEC 编码
	m_fec_encoder.WriteSegmentData(buf);
}

//------------------------------------------------------------------------------
// IFeedbackHandler
//------------------------------------------------------------------------------
void StreamServer::OnNackRequest(uint32_t channel_id, uint32_t user_id, 
	const std::vector<uint32_t> nacks, std::vector<com::Buffer>& pkts)
{
	com::Buffer buf;

	for (auto seq : nacks) {
		if (m_nack_history.FindFecFrameData(seq, buf)) {
			pkts.push_back(buf);
		}
		else {
			LOG_WRN("Cannot find nack packet, seq:{}", seq);
		}
	}
}

//------------------------------------------------------------------------------
// IFeedbackHandler
//------------------------------------------------------------------------------
void StreamServer::OnStateFeedback(uint32_t channel_id, uint32_t user_id, 
	const StateFeedback& feedback)
{
	// FIXME: 这里应该传入所有接收端的统计数据

	StateFB fb;
	fb.start_time = feedback.start_time;
	fb.end_time   = feedback.end_time;
	fb.recv_count = feedback.recv_count;
	fb.lost_count = feedback.lost_count;
	fb.rtt = feedback.rtt;
	fb.olr = feedback.olr;
	fb.flr = feedback.flr;
	fb.nlr = feedback.nlr;
	fb.clc = feedback.clc;
	fb.flc = feedback.flc;
	fb.fc  = feedback.fc;	

	m_fec_param_controller.OnStateFeedback(fb);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void StreamServer::OnFecParamUpdate(uint8_t k, uint8_t r)
{
	m_fec_encoder.SetParam(k, r);
}

}