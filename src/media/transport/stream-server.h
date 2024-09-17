#pragma once

#include <unordered_map>
#include <mutex>

#include "com-factory.h"

#include "if-timer-mgr.h"
#include "if-stream-server.h"
#include "server-negotiator.h"
#include "transport-common.h"
#include "fec-encoder.h"
#include "frame-packer.h"
#include "nack-history.h"
#include "fec-param-controller.h"
#include "seq-allocator.h"

namespace jukey::txp
{

//==============================================================================
// 
//==============================================================================
class StreamServer 
	: public IStreamServer
	, public IStreamReceiverHandler
	, public IStreamSenderHandler
	, public IFecEncodeHandler
	, public IFramePackHandler
	, public IFeedbackHandler
	, public IFecParamHandler
{
public:
	StreamServer(base::IComFactory* factory, 
		IServerHandler* handler, 
		util::IThread* thread,
		const com::MediaStream& stream);
	~StreamServer();

	// IStreamServer
	virtual uint32_t SrcChannelId() override;
	virtual uint32_t SrcUserId() override;
	virtual bool HasDstChannel(uint32_t channel_id) override;
	virtual uint32_t DstChannelSize() override;
	virtual com::Stream Stream() override;
	virtual com::ErrCode SetStreamSender(uint32_t channel_id,
		uint32_t user_id) override;
	virtual void RemoveStreamSender() override;
	virtual com::ErrCode AddStreamReceiver(uint32_t channel_id, 
		uint32_t user_id) override;
	virtual com::ErrCode RemoveStreamReceiver(uint32_t channel_id) override;
	virtual void OnRecvChannelData(uint32_t channel_id, uint32_t mt,
		const com::Buffer& buf) override;
	virtual void OnRecvChannelMsg(uint32_t channel_id,
		const com::Buffer& buf) override;

	// IStreamReceiverHandler
	virtual void OnStreamFrame(uint32_t channel_id,
		uint32_t user_id,
		const com::MediaStream& stream,
		const com::Buffer& buf) override;
	virtual void OnReceiverFeedback(uint32_t channel_id,
		uint32_t user_id,
		const com::MediaStream& stream,
		const com::Buffer& buf) override;
	
	// IStreamSenderHandler
	virtual void OnStreamData(uint32_t channel_id,
		uint32_t user_id,
		const com::MediaStream& stream,
		const com::Buffer& buf) override;
	virtual void OnSenderFeedback(uint32_t channel_id,
		uint32_t user_id,
		const com::MediaStream& stream,
		const com::Buffer& buf) override;
	virtual void OnEncoderTargetBitrate(uint32_t channel_id,
		uint32_t user_id,
		const com::MediaStream& stream, 
		uint32_t bw_kbps) override;

	// IFecEncodeHandler
	virtual void OnFecFrameData(const com::Buffer& buf) override;

	// IFramePackHandler
	virtual void OnSegmentData(const com::Buffer& buf) override;

	// IFeedbackHandler
	virtual void OnNackRequest(uint32_t channel_id, 
		uint32_t user_id, 
		const std::vector<uint32_t> nacks,
		std::vector<com::Buffer>& pkts) override;
	virtual void OnStateFeedback(uint32_t channel_id, 
		uint32_t user_id, 
		const StateFeedback& feedback) override;

	// IFecParamHandler
	virtual void OnFecParamUpdate(uint8_t k, uint8_t r) override;

private:
	void OnRecvStreamData(uint32_t channel_id, const com::Buffer& buf);
	void OnRecvFeedback(uint32_t channel_id, const com::Buffer& buf);

private:
	friend class ServerNegotiator;

private:
	base::IComFactory* m_factory = nullptr;
	util::IThread* m_thread = nullptr;
	com::ITimerMgr* m_timer_mgr = nullptr;
	IServerHandler* m_handler = nullptr;
	
	com::MediaStream m_stream;
	
	ReceiverInfo m_receiver;

	NegoState m_nego_state = NegoState::NEGO_STATE_INIT;

	// channel:sender
	std::unordered_map<uint32_t, SenderInfoSP> m_senders;
	std::mutex m_mutex;

	ServerNegotiatorSP m_negotiator;

	FecEncoder m_fec_encoder;
	FramePacker m_frame_packer;
	NackHistory m_nack_history;
	SeqAllocator m_seq_allocator;
	SimpleFecParamController m_fec_param_controller;
};

}