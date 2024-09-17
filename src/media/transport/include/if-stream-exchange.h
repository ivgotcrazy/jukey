#pragma once

#include <string>

#include "if-unknown.h"
#include "thread/if-thread.h"

#include "common-struct.h"

namespace jukey::txp
{

#define CID_STREAM_EXCHAGE "cid-stream-exchange"
#define IID_STREAM_EXCHAGE "iid-stream-exchange"

//==============================================================================
// Stream handler
//==============================================================================
class IExchangeHandler
{
public:
	//
	// @brief Channel data to be transfered
	//
	virtual void OnSendChannelMsg(uint32_t channel_id, uint32_t user_id, 
		const com::Buffer& buf) = 0;

	virtual void OnSendChannelData(uint32_t channel_id, uint32_t user_id,
		uint32_t mt, const com::Buffer& buf) = 0;
};

//==============================================================================
// Stream exchange
//==============================================================================
class IStreamExchange : public base::IUnknown
{
public:
	//
	// @brief Initialize stream exchange
	//
	virtual com::ErrCode Init(IExchangeHandler* handler, util::IThread* thread) = 0;

	//
	// @brief Check stream existance
	//
	virtual bool HasStream(const std::string& stream_id) = 0;

	//
	// @brief Check stream sender existance
	//
	virtual bool HasSender(const std::string& stream_id) = 0;

	//
	// @brief Add a src stream
	// @param stream     stream info
	// @param channel_id src channel ID
	//
	virtual com::ErrCode AddSrcChannel(const com::MediaStream& stream, 
		uint32_t channel_id) = 0;

	//
	// @brief Remove a src stream
	// @param channel_id src channel ID
	//
	virtual com::ErrCode RemoveSrcChannel(uint32_t channel_id) = 0;

	//
	// @brief Add a dst stream
	// @param stream     stream info
	// @param channel_id dst channel ID
	// @param user_id    user ID binding to dst channel
	virtual com::ErrCode AddDstChannel(const com::MediaStream& stream,
		uint32_t channel_id,
		uint32_t user_id) = 0;

	//
	// @brief Remove a dst stream
	// @param channel_id stream receive user
	// @param stream_id  stream ID
	//
	virtual com::ErrCode RemoveDstChannel(uint32_t channel_id,
		const std::string& stream_id) = 0;

	//
	// @brief Received stream data from src stream
	// @param channel_id channel ID
	// @param mt         message type
	// @param buf        stream data
	//
	virtual void OnRecvChannelData(uint32_t channel_id, uint32_t mt,
		const com::Buffer& buf) = 0;

	virtual void OnRecvChannelMsg(uint32_t channel_id,
		const com::Buffer& buf) = 0;
};

}