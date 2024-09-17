#pragma once

#include <memory>

#include "thread/if-thread.h"
#include "common-struct.h"


namespace jukey::txp
{

//==============================================================================
// 
//==============================================================================
class IServerHandler
{
public:
	//
	// Channel data need to be send
	//
	virtual void OnSendChannelMsg(uint32_t channel_id, 
		uint32_t user_id,
		const com::Buffer& buf) = 0;

	virtual void OnSendChannelData(uint32_t channel_id, 
		uint32_t user_id,
		uint32_t mt, 
		const com::Buffer& buf) = 0;
};

//==============================================================================
// 
//==============================================================================
class IStreamServer
{
public:
	virtual ~IStreamServer() {}

	//
	// Get receive channel ID
	//
	virtual uint32_t SrcChannelId() = 0;

	//
	// Whether dst channel exists
	//
	virtual bool HasDstChannel(uint32_t channel_id) = 0;

	//
	// How many dst channels
	//
	virtual uint32_t DstChannelSize() = 0;

	//
	// Get send user ID
	//
	virtual uint32_t SrcUserId() = 0;

	//
	// Get stream info
	//
	virtual com::Stream Stream() = 0;

	//
	// Add stream sender
	//
	virtual com::ErrCode SetStreamSender(uint32_t channel_id, 
		uint32_t user_id) = 0;

	//
	// Remove stream sender
	//
	virtual void RemoveStreamSender() = 0;

	//
	// Add stream receiver
	//
	virtual com::ErrCode AddStreamReceiver(uint32_t channel_id, 
		uint32_t user_id) = 0;

	// 
	// Remove stream receiver
	//
	virtual com::ErrCode RemoveStreamReceiver(uint32_t channel_id) = 0;

	//
	// Input received channel data
	//
	virtual void OnRecvChannelData(uint32_t channel_id, uint32_t mt,
		const com::Buffer& buf) = 0;

	virtual void OnRecvChannelMsg(uint32_t channel_id,
		const com::Buffer& buf) = 0;
};
typedef std::shared_ptr<IStreamServer> IStreamServerSP;

}