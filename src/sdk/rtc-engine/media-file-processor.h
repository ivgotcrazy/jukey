#pragma once

#include <map>

#include "common-struct.h"
#include "common-error.h"

namespace jukey::sdk
{

class RtcEngineImpl;

//==============================================================================
// 
//==============================================================================
class MediaFileProcessor
{
public:
	MediaFileProcessor(RtcEngineImpl* engine);

	com::ErrCode Open(const std::string& file, void* wnd);
	com::ErrCode Close(const std::string& file);

	void OnAddMediaStream(const com::MediaStream& stream);
	void OnDelMediaStream(const com::MediaStream& stream);

private:
	RtcEngineImpl* m_rtc_engine = nullptr;
	std::string m_media_file;
	std::unique_ptr<com::MediaStream> m_audio_stream;
	std::unique_ptr<com::MediaStream> m_video_stream;
	void* m_wnd = nullptr;
};

}