#pragma once

#include "common-struct.h"
#include "if-unknown.h"

#include "../../public/media-struct.h"

namespace jukey::txp
{

#define CID_STREAM_RECEIVER "cid-stream-receiver"
#define IID_STREAM_RECEIVER "iid-stream-receiver"

//==============================================================================
// 
//==============================================================================
class IStreamReceiverHandler
{
public:
	//
	// @brief Received audio/video frame
	//
	virtual void OnStreamFrame(uint32_t channel_id,
		uint32_t user_id,
		const com::MediaStream& stream,
		const com::Buffer& buf) = 0;

	//
	// @brief Feedback data should send to sender
	//
	virtual void OnReceiverFeedback(uint32_t channel_id,
		uint32_t user_id,
		const com::MediaStream& stream,
		const com::Buffer& buf) = 0;
};

//==============================================================================
// 
//==============================================================================
class IStreamReceiver : public base::IUnknown
{
public:
	//
	// @brief Initialize stream receiver
	//
	virtual com::ErrCode Init(IStreamReceiverHandler* handler,
		uint32_t channel_id,
		uint32_t user_id,
		const com::MediaStream& stream) = 0;

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
	// Input stream data
	//
	virtual void InputStreamData(const com::Buffer& buf) = 0;

	//
	// Input feedback
	//
	virtual void InputFeedback(const com::Buffer& buf) = 0;
};

}