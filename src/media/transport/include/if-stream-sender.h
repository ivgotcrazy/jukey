#pragma once

#include "common-struct.h"
#include "if-unknown.h"
#include "protocol.h"
#include "../../public/media-struct.h"

namespace jukey::txp
{

#define CID_STREAM_SENDER "cid-stream-sender"
#define IID_STREAM_SENDER "iid-stream-sender"

#define CID_SERVER_STREAM_SENDER "cid-server-stream-sender"
#define IID_SERVER_STREAM_SENDER "iid-server-stream-sender"

//==============================================================================
// 
//==============================================================================
class IStreamSenderHandler
{
public:
	//
	// @brief Stream data to be sent to receiver 
	//
	virtual void OnStreamData(uint32_t channel_id,
		uint32_t user_id,
		const com::MediaStream& stream,
		const com::Buffer& buf) = 0;

	//
	// @brief Feedback data recevied from receiver 
	//
	virtual void OnSenderFeedback(uint32_t channel_id,
		uint32_t user_id, 
		const com::MediaStream& stream, 
		const com::Buffer& buf) = 0;

	virtual void OnEncoderTargetBitrate(uint32_t channel_id,
		uint32_t user_id,
		const com::MediaStream& stream, 
		uint32_t bw_kbps) = 0;
};

//==============================================================================
// 
//==============================================================================
struct StateFeedback
{
	uint32_t start_time;
	uint32_t end_time;
	uint32_t recv_count;
	uint32_t lost_count;
	uint16_t rtt; // round trip time
	uint8_t olr;  // original loss rate
	uint8_t flr;  // fec loss rate
	uint8_t nlr;  // nack loss rate
	uint8_t clc;  // maximum consecutive loss count
	uint8_t flc;  // frame loss count
	uint8_t fc;   // frame count
};

//==============================================================================
// 
//==============================================================================
struct BitrateConfig
{
	uint32_t min_bitrate_kbps = 0;
	uint32_t max_bitrate_kbps = 0;
	uint32_t start_bitrate_kbps = 0;
};

//==============================================================================
// 
//==============================================================================
class IStreamSender : public base::IUnknown
{
public:
	//
	// @brief Initialize stream sender
	//
	virtual com::ErrCode Init(IStreamSenderHandler* handler, 
		uint32_t channel_id,
		uint32_t user_id, 
		const com::MediaStream& stream) = 0;

	//
	// Set bitrate configration
	//
	virtual bool SetBitrateConfig(const BitrateConfig& config) = 0;

	//
	// Stream
	//
	virtual com::Stream Stream() = 0;

	//
	// Get channel ID
	//
	virtual uint32_t ChannelId() = 0;

	//
	// Get user ID
	//
	virtual uint32_t UserId() = 0;

	//
	// Input audio/video frame
	//
	virtual void InputFrameData(const com::Buffer& buf) = 0;

	//
	// Input feedback data
	//
	virtual void InputFeedbackData(const com::Buffer& buf) = 0;
};

//==============================================================================
// 
//==============================================================================
class IFeedbackHandler
{
public:
	virtual void OnNackRequest(uint32_t channel_id,
		uint32_t user_id,
		const std::vector<uint32_t> nacks,
		std::vector<com::Buffer>& pkts) = 0;

	virtual void OnStateFeedback(uint32_t channel_id,
		uint32_t user_id,
		const StateFeedback& feedback) = 0;
};

//==============================================================================
// 
//==============================================================================
class IServerStreamSender : public base::IUnknown 
{
public:
	//
	// @brief Initialize stream sender
	//
	virtual com::ErrCode Init(IStreamSenderHandler* sender_handler, 
		IFeedbackHandler* feedback_handler,
		uint32_t channel_id,
		uint32_t user_id, 
		const com::MediaStream& stream) = 0;

	//
	// Set bitrate configration
	//
	virtual bool SetBitrateConfig(const BitrateConfig& config) = 0;

	//
	// Stream
	//
	virtual com::Stream Stream() = 0;

	//
	// Channel ID
	//
	virtual uint32_t ChannelId() = 0;

	//
	// User ID
	//
	virtual uint32_t UserId() = 0;

	//
	// Input FEC frame data
	//
	virtual void InputFecFrameData(const com::Buffer& buf) = 0;

	//
	// Input feedback data
	//
	virtual void InputFeedbackData(const com::Buffer& buf) = 0;
};

}