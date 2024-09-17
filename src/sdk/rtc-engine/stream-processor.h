#pragma once

#include <map>

#include "common-struct.h"

namespace jukey::sdk
{

class RtcEngineImpl;

//==============================================================================
// 
//==============================================================================
class StreamProcessor
{
public:
	StreamProcessor(RtcEngineImpl* engine);

	com::ErrCode StartRecvVideo(const com::MediaStream& stream, void* wnd);
	com::ErrCode StopRecvVideo(const com::MediaStream& stream, void* wnd);

	com::ErrCode StartRecvAudio(const com::MediaStream& stream);
	com::ErrCode StopRecvAudio(const com::MediaStream& stream);

	void OnAddMediaStream(const com::MediaStream& stream);

private:
	com::ErrCode SubscribeStream(const com::MediaStream& stream);
	com::ErrCode UnsubscribeStream(const com::MediaStream& stream);

	void OnSubStreamRsp(const com::MediaStream& stream, const com::Buffer& buf);
	void OnSubStreamTimeout(const com::MediaStream& stream);
	void OnSubStreamError(const com::MediaStream& stream, com::ErrCode ec);

	void OnUnsubStreamRsp(const com::MediaStream& stream, const com::Buffer& buf);
	void OnUnsubStreamTimeout(const com::MediaStream& stream);
	void OnUnsubStreamError(const com::MediaStream& stream, com::ErrCode ec);

private:
	struct VideoEntry
	{
		com::MediaStream param;
		void* wnd = nullptr;
	};

private:
	RtcEngineImpl* m_rtc_engine = nullptr;
	
	// key: stream ID
	std::map<std::string, VideoEntry> m_video_entries;
};

}