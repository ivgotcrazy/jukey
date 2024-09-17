#pragma once

#include <mutex>

#include "include/if-stream-sender.h"
#include "if-unknown.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "thread/common-thread.h"
#include "frame-packer.h"
#include "pacing-sender.h"
#include "if-stream-server.h"
#include "pacing-sender.h"
#include "if-congestion-controller.h"
#include "seq-allocator.h"

namespace jukey::txp
{

//==============================================================================
// TODO: thread
//==============================================================================
class ServerStreamSender 
	: public IServerStreamSender
	, public base::ProxyUnknown
	, public base::ComObjTracer
	, public IPacingSenderHandler
	, public cc::IBandwidthObserver
	, public cc::IPacketSender
{
public:
	ServerStreamSender(base::IComFactory* factory, const char* owner);
	~ServerStreamSender();

	// ProxyUnknown
	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IStreamSender
	virtual com::ErrCode Init(IStreamSenderHandler* sender_handler,
		IFeedbackHandler* feedback_handler, 
		uint32_t channel_id,
		uint32_t user_id,
		const com::MediaStream& stream) override;
	virtual bool SetBitrateConfig(const BitrateConfig& config) override;
	virtual com::Stream Stream() override;
	virtual uint32_t ChannelId() override;
	virtual uint32_t UserId() override;
	virtual void InputFecFrameData(const com::Buffer& buf) override;
	virtual void InputFeedbackData(const com::Buffer& buf) override;

	// IPacingSenderHandler
	virtual void OnPacingData(const com::Buffer& buf) override;

	// IBandwidthObserver
	virtual void OnBandwidthUpdate(uint32_t bw_kbps) override;

	// IPacketSender
	virtual void SendPacket(const com::Buffer& buf, 
		const cc::PacingInfo& info) override;
	virtual std::vector<com::Buffer> GeneratePadding(uint32_t data_size) override;

private:
	void OnStateFeedback(const com::Buffer& buf);
	void OnNackFeedback(const com::Buffer& buf);
	void OnRttRequest(const com::Buffer& buf);
	void OnAddPacket(const com::Buffer& buf);
	void OnSentPacket(const com::Buffer& buf);
	void OnTransportFeedback(const com::Buffer& buf);

private:
	base::IComFactory* m_factory = nullptr;
	uint32_t m_channel_id = 0;
	uint32_t m_user_id = 0;
	com::MediaStream m_stream;
	IStreamSenderHandler* m_sender_handler = nullptr;
	IFeedbackHandler* m_feedback_handler = nullptr;
	std::mutex m_mutex;
	uint32_t m_seq = 0; // TODO: wrap around
	PacingSender m_pacing_sender;
	cc::ICongetionController* m_congestion_controller = nullptr;
	SeqAllocator m_seq_allocator;
};

}
