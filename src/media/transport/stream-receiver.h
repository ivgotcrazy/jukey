#pragma once

#include <mutex>
#include <set>

#include "include/if-stream-receiver.h"
#include "if-unknown.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "thread/common-thread.h"
#include "common/util-dump.h"
#include "fec-decoder.h"
#include "frame-unpacker.h"
#include "if-timer-mgr.h"
#include "nack-requester.h"
#include "common/util-stats.h"

namespace jukey::txp
{

//==============================================================================
// 
//==============================================================================
class StreamReceiver 
	: public IStreamReceiver
	, public base::ProxyUnknown
	, public base::ComObjTracer
	, public IFecDecodeHandler
	, public IFrameUnpackHandler
	, public INackRequestHandler
{
public:
	StreamReceiver(base::IComFactory* factory, const char* owner);
	~StreamReceiver();

	// ProxyUnknown
	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IStreamReceiver
	virtual com::ErrCode Init(IStreamReceiverHandler* handler, uint32_t user_id,
		uint32_t channel_id, const com::MediaStream& stream) override;
	virtual com::Stream Stream() override;
	virtual uint32_t ChannelId() override;
	virtual uint32_t UserId() override;
	virtual void InputStreamData(const com::Buffer& buf) override;
	virtual void InputFeedback(const com::Buffer& buf) override;

	//virtual void OnRecvStreamData(const com::Buffer& buf) override;

	// IFecDecoderHandler
	virtual void OnSegmentData(const com::Buffer& buf) override;

	// IFrameUnpackHandler
	virtual void OnFrameData(const com::Buffer& buf) override;

	// INackRequestHandler
	virtual void OnNackRequest(uint32_t count, const com::Buffer& buf) override;

private:
	struct LossStats
	{
		uint32_t start_time = 0;
		uint32_t end_time = 0;
		uint32_t recv_count = 0;
		uint32_t lost_count = 0;
		uint32_t loss_rate = 0;
		uint32_t max_consecutive_loss = 0;
	};

	struct FrameStats
	{
		uint32_t frame_count = 0;
		uint32_t frame_loss = 0;
	};

private:
	void OnTimer();
	void OnRttResponse(const com::Buffer& buf);

	void SendStateFeedback();
	void SendRttRequest();
	void SendTransportFeedback();

	void InitTimer();
	void InitStats();

	LossStats CalcOriginLossStats();
	FrameStats CalcFrameStats();

	com::Buffer BuildTransportFeedback();

private:
	base::IComFactory* m_factory = nullptr;
	IStreamReceiverHandler* m_handler = nullptr;
	uint32_t m_channel_id = 0;
	uint32_t m_user_id = 0;
	com::MediaStream m_stream;
	std::mutex m_mutex;
	
	com::Buffer m_video_frame_buf;

	FecDecoder m_fec_decoder;
	IFrameUnpackerUP m_frame_unpacker;

	// 补偿前丢包统计(seq) - before recover
	std::set<uint32_t> m_b_rcvr_pkt;
	uint64_t m_start_time_us = 0;

	// 补偿后丢帧统计(fseq)
	std::set<uint32_t> m_a_rcvr_frm;

	com::ITimerMgr* m_timer_mgr = nullptr;
	com::TimerId m_timer_id = 0;

	NackRequesterUP m_nack_requester;

	// Statistics
	util::DataStatsSP m_data_stats;
	util::StatsId m_i_rtx_send = INVALID_STATS_ID;
	util::StatsId m_i_rtx_recv = INVALID_STATS_ID;
	util::StatsId m_i_rtx_br = INVALID_STATS_ID;

	uint32_t m_rtt_value = 0;
	uint32_t m_last_send_rtt_seq = 0;
	uint32_t m_last_recv_rtt_seq = 0;

	std::map<uint32_t, uint64_t> m_received_pkts;

	uint8_t m_feedback_sn = 0;
};

}
