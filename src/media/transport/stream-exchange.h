#pragma once

#include <unordered_map>
#include <vector>
#include <set>
#include <mutex>

#include "include/if-stream-exchange.h"
#include "include/if-stream-server.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"
#include "thread/common-thread.h"

namespace jukey::txp
{
	
//==============================================================================
// 
//==============================================================================
class StreamExchange 
	: public IStreamExchange
	, public base::ProxyUnknown
	, public base::ComObjTracer
	, public IServerHandler
{
public:
	StreamExchange(base::IComFactory* factory, const char* owner);

	// ProxyUnknown
	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IStreamExchange
	virtual com::ErrCode Init(IExchangeHandler* handler,
		util::IThread* thread) override;
	virtual bool HasStream(const std::string& stream_id) override;
	virtual bool HasSender(const std::string& stream_id) override;
	virtual com::ErrCode AddSrcChannel(const com::MediaStream& stream,
		uint32_t channel_id) override;
	virtual com::ErrCode RemoveSrcChannel(uint32_t channel_id) override;
	virtual com::ErrCode AddDstChannel(const com::MediaStream& stream,
		uint32_t channel_id, uint32_t user_id) override;
	virtual com::ErrCode RemoveDstChannel(uint32_t channel_id,
		const std::string& stream_id) override;
	virtual void OnRecvChannelData(uint32_t channel_id, uint32_t mt,
		const com::Buffer& buf) override;
	virtual void OnRecvChannelMsg(uint32_t channel_id,
		const com::Buffer& buf) override;

	// IServerHandler
	virtual void OnSendChannelMsg(uint32_t channel_id, uint32_t user_id,
		const com::Buffer& buf) override;
	virtual void OnSendChannelData(uint32_t channel_id, uint32_t user_id,
		uint32_t mt, const com::Buffer& buf) override;

private:
	IStreamServerSP FindOrCreateStreamServer(const com::MediaStream& stream,
		uint32_t channel_id);

private:
	base::IComFactory* m_factory = nullptr;
	util::IThread* m_thread = nullptr;
	IExchangeHandler* m_handler = nullptr;

	// channel(src channel and dst channel):IStreamServer
	std::unordered_map<uint32_t, IStreamServerSP> m_stream_servers;
	std::mutex m_mutex;
};

}
