#pragma once

#include <memory>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <future>

#include "common-enum.h"
#include "common-define.h"
#include "common-error.h"

namespace jukey::com
{

//==============================================================================
// 
//==============================================================================
struct Buffer
{
	Buffer() {}

	Buffer(uint32_t len) : total_len(len)
	{
		data.reset(new uint8_t[total_len]);
		memset(data.get(), 0, total_len);
	}

	Buffer(uint32_t tl, uint32_t dl) : total_len(tl), data_len(dl)
	{
		data.reset(new uint8_t[total_len]);
		memset(data.get(), 0, total_len);
	}

	Buffer(const char* msg) : Buffer((uint32_t)strlen(msg))
	{
		memcpy(data.get(), msg, total_len);
		data_len = total_len;
	}

	Buffer(uint8_t* src, uint32_t len) : Buffer(len)
	{
		memcpy(data.get(), src, len);
		data_len = len;
	}

	Buffer(uint32_t max_len, uint8_t* src, uint32_t src_len, uint32_t pos)
		: Buffer(max_len)
	{
		memcpy(data.get() + pos, src, src_len);
		data_len = src_len;
		start_pos = pos;
	}

	Buffer(char* src, uint32_t len) : Buffer((uint8_t*)src, len) {}

	Buffer(const char* src, uint32_t len) : Buffer((uint8_t*)src, len) {}

	Buffer Clone() const
	{
		Buffer buf;
		buf.total_len = total_len;
		buf.data_len = data_len;
		buf.start_pos = start_pos;

		buf.data.reset(new uint8_t[total_len]);
		memcpy(buf.data.get(), data.get(), total_len);

		return buf;
	}

	// Data
	std::shared_ptr<uint8_t> data;

	// Total data length
	uint32_t total_len = 0;

	// Where the data begin from
	uint32_t start_pos = 0;

	// Actually used data length
	uint32_t data_len = 0;
};
typedef std::shared_ptr<Buffer> BufferSP;

//==============================================================================
// 
//==============================================================================
struct CommonMsg
{
	CommonMsg() {}

	CommonMsg(uint32_t mt) : msg_type(mt) {}

	CommonMsg(uint32_t mt, uint32_t p) : msg_type(mt), pri(p) {}

	CommonMsg(uint32_t mt, const std::shared_ptr<void> md)
		: msg_type(mt), msg_data(md) {}

	CommonMsg(uint32_t mt, uint32_t p, const std::shared_ptr<void> md)
		: msg_type(mt), pri(p), msg_data(md) {}

	// Message type
	uint32_t msg_type = 0;

	// 0: no priority, higher value means higher priority
	uint32_t pri = 0;
	
	// Where the message send from, optional
	std::string src;

	// Where the message send to, optional
	std::string dst;

	// Message data
	std::shared_ptr<void> msg_data;

	// Return message process result synchronously
	PRECSP result;
};
typedef std::shared_ptr<CommonMsg> CommonMsgSP;

//==============================================================================
// 
//==============================================================================
struct DoubleBuffer
{
	DoubleBuffer(const com::Buffer& b1, const com::Buffer& b2)
		: buf1(b1), buf2(b2) {}

	com::Buffer buf1;
	com::Buffer buf2;
};

//==============================================================================
// Media source
//==============================================================================
struct MediaSrc
{
	MediaSrc() {}

	MediaSrc(MediaSrcType type, const std::string& id) 
		: src_type(type), src_id(id) {}

	bool operator<(const MediaSrc& src) const
	{
		if (src_type < src.src_type) {
			return true;
		}
		else if (src_type == src.src_type) {
			return src_id < src.src_id;
		}
		else {
			return false;
		}
	}

	std::string ToStr() const
	{
		switch (src_type) {
		case MediaSrcType::CAMERA:
			return std::string().append("cam://")
				.append(std::to_string(app_id))
				.append("/")
				.append(std::to_string(user_id))
				.append("/")
				.append(src_id);
		case MediaSrcType::MICROPHONE:
			return std::string().append("mic://")
				.append(std::to_string(app_id))
				.append("/")
				.append(std::to_string(user_id))
				.append("/")
				.append(src_id);
		case MediaSrcType::FILE:
			return std::string().append("file://")
				.append(std::to_string(app_id))
				.append("/")
				.append(std::to_string(user_id))
				.append("/")
				.append(src_id);
		case MediaSrcType::RTSP:
			return src_id; // rtsp://xxxx
		default:
			return std::string();
		}
	}

	MediaSrcType src_type = MediaSrcType::INVALID;
	std::string src_id;
	uint32_t app_id = 0; // 0: local
	uint32_t user_id = 0; // 0: local
};

//==============================================================================
// Identify a stream with ID and type
//==============================================================================
struct Stream
{
	Stream() {}

	Stream(const std::string& id, StreamType type)
		: stream_id(id), stream_type(type) {}

	Stream(const std::string& id, uint32_t type)
		: stream_id(id), stream_type((StreamType)type) {}

	std::string ToStr()
	{
		return std::to_string((uint32_t)stream_type) + "|" + stream_id;
	}

	StreamType stream_type = StreamType::INVALID;
	std::string stream_id;
};

//==============================================================================
// 
//==============================================================================
struct MediaStream
{
	Stream stream;
	MediaSrc src;
};

//==============================================================================
// 
//==============================================================================
struct ElementPin
{
	std::string pl_name;
	std::string ele_name;
	std::string pin_name;
};

//==============================================================================
// Element stream information
//==============================================================================
struct ElementStream
{
	MediaStream stream;
	ElementPin pin;
	std::string cap;
};

//==============================================================================
// 
//==============================================================================
struct AudioStreamStats
{
	uint32_t bitrate = 0;
	uint32_t rtt = 0;
	uint32_t loss_rate = 0;
};

//==============================================================================
// 
//==============================================================================
struct VideoStreamStats
{
	uint32_t bitrate = 0;
	uint32_t frame_rate = 0;
	uint32_t rtt = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t loss_rate = 0;
};

//==============================================================================
// Media device
//==============================================================================
struct DevItem
{
	DevType dev_type = DevType::INVALID;

	std::string dev_id;
	std::string dev_name;
	std::string dev_desc;

	void* dev_data = nullptr;
};

//==============================================================================
// 
//==============================================================================
struct CamParam
{
	uint32_t dev_id = 0;
	uint32_t resolution = 0;
	uint32_t frame_rate = 0;
	uint32_t pixel_format = 0;
};

//==============================================================================
// 
//==============================================================================
struct MicParam
{
	uint32_t dev_id = 0;
	uint32_t sample_chnl = 0;
	uint32_t sample_rate = 0;
	uint32_t sample_bits = 0;
};

//==============================================================================
// Host and port pair
//==============================================================================
struct Endpoint
{
	Endpoint() {}
	Endpoint(CSTREF i, uint16_t p) : host(i), port(p) {}

	bool operator==(const Endpoint& ep) const
	{
		return (ep.host == host && ep.port == port);
	}

	bool operator<(const Endpoint& ep) const
	{
		return (host < ep.host || port < ep.port);
	}

	std::string ToStr() const
	{
		return std::string(host).append(":").append(std::to_string(port));
	}

	std::string host; // TODO: performance???
	uint16_t port = 0;
};

//==============================================================================
// Address format：protocol:ip:port
// example1: TCP:127.0.0.1:8888
// example2: TCP:example.com:8888
//==============================================================================
struct Address
{
	Address() {}
	Address(const Endpoint& e, AddrType t) : ep(e), type(t) {}
	Address(CSTREF ip, uint16_t p, AddrType t) : ep(ip, p), type(t) {}

	bool operator==(const Address& addr) const
	{
		return (addr.ep == ep && addr.type == type);
	}

	std::string ToStr() const
	{
		if (type == AddrType::TCP) {
			return std::string("TCP:").append(ep.ToStr());
		}
		else if (type == AddrType::UDP) {
			return std::string("UDP:").append(ep.ToStr());
		}
		else {
			return std::string();
		}
	}

	Endpoint ep;
	AddrType type = AddrType::INVALID;
};

//==============================================================================
// 
//==============================================================================
struct SigHdrParam 
{
	SigHdrParam() {}
	
	SigHdrParam(uint32_t s, uint32_t app, uint32_t group, uint32_t user, 
		uint32_t client)
		: seq(s), app_id(app), group_id(group), user_id(user), client_id(client) 
	{}

	uint32_t seq = 0;
	uint32_t app_id = 0;
	uint32_t group_id = 0;
	uint32_t user_id = 0;
	uint32_t client_id = 0;
	bool clear_group = false;
	bool clear_user = false;
	bool clear_client = false;
};

//==============================================================================
// 
//==============================================================================
class MainThreadExecutor
{
public:
	virtual void RunInMainThread(std::function<void()> task) = 0;
};

}
